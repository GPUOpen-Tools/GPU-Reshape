#pragma once

// Layer
#include <Backends/DX12/Compiler/DXBC/DXBCParseContext.h>
#include <Backends/DX12/Compiler/DXBC/MSF/MSFStructure.h>
#include <Backends/DX12/Compiler/DXBC/MSF/MSFHeader.h>

// Std
#include <cstring>

struct MSFParseContext {
    /// Constructor
    /// \param data given data
    /// \param length byte size of data
    /// \param allocators allocators
    MSFParseContext(const void* data, size_t length, const Allocators& allocators) : directory(allocators), ctx(data, length), allocators(allocators) {

    }

    /// Parse this MSF
    /// \return true if succeeded
    bool Parse() {
        super = ctx.Consume<MSFSuperBlock>();

        // Validate magic
        if (std::memcmp(super.magic, kMSFSuperBlockMagic, sizeof(kMSFSuperBlockMagic)) != 0) {
            ASSERT(false, "MSF block header validation failed");
            return false;
        }

        /** Skip FPM blocks */

        // Compute block counts
        const uint32_t numDirectoryBlocks = GetBlockCount(super.directoryByteCount);
        const uint32_t numAddressBlocks   = GetBlockCount(numDirectoryBlocks * sizeof(uint32_t));

        // Get address block
        DXBCParseContext blockAddresses = GetBlockAt(super.blockMapAddr, numAddressBlocks);

        // Flattened stream directory block
        Vector<uint8_t> directoryBlock(allocators);
        directoryBlock.resize(super.directoryByteCount);

        // Flatten stream directory
        for (size_t remaining = super.directoryByteCount, directoryBlockIndex = 0; directoryBlockIndex < numDirectoryBlocks; directoryBlockIndex++) {
            uint32_t blockAddr = blockAddresses.Consume<uint32_t>();

            // Copy
            const uint32_t* ptr = GetBlockAt(blockAddr);
            std::memcpy(&directoryBlock[super.blockSize * directoryBlockIndex], ptr, std::min<size_t>(remaining, super.blockSize));
            remaining -= super.blockSize;
        }

        // Setup context
        DXBCParseContext streamDirectory(directoryBlock.data(), directoryBlock.size());

        // Read stream count
        uint32_t streamCount = streamDirectory.Consume<uint32_t>();

        // Preallocate files
        for (uint32_t streamIndex = 0; streamIndex < streamCount; streamIndex++) {
            MSFFile& file = directory.files.emplace_back(allocators);
            file.data.resize(streamDirectory.Consume<uint32_t>());
        }

        // Read all files
        for (uint32_t streamIndex = 0; streamIndex < streamCount; streamIndex++) {
            MSFFile& file = directory.files[streamIndex];

            // Byte count to block count
            uint32_t blockCount = GetBlockCount(file.data.size());

            // Read all contents
            for (size_t remaining = file.data.size(), blockIndex = 0; blockIndex < blockCount; blockIndex++) {
                auto blockOffset = streamDirectory.Consume<uint32_t>();

                // Copy
                const uint32_t* ptr = GetBlockAt(blockOffset);
                std::memcpy(&file.data[super.blockSize * blockIndex], ptr, std::min<size_t>(remaining, super.blockSize));
                remaining -= super.blockSize;
            }
        }

        // OK
        return true;
    }

    /// Get the root directory
    const MSFDirectory& GetDirectory() const {
        return directory;
    }

private:
    /// Get a block at
    /// \param offset given offset
    /// \return data
    const uint32_t* GetBlockAt(uint32_t offset) {
        return ctx.ReadAt<uint32_t>(super.blockSize * offset);
    }

    /// Get a block at
    /// \param offset given block offset
    /// \param count number of blocks
    /// \return parsing context
    DXBCParseContext GetBlockAt(uint32_t offset, uint32_t count) {
        const uint32_t* value = ctx.ReadAt<uint32_t>(super.blockSize * offset);
        return DXBCParseContext(value, super.blockSize * count);
    }

    /// Get the number of blocks for a given byte size
    uint32_t GetBlockCount(size_t size) const {
        return static_cast<uint32_t>((size + (super.blockSize - 1)) / super.blockSize);
    }

private:
    /// Root directory
    MSFDirectory directory;

private:
    /// Parsing context
    DXBCParseContext ctx;

    /// Shared allocators
    Allocators allocators;

    /// Super block
    MSFSuperBlock super;
};
