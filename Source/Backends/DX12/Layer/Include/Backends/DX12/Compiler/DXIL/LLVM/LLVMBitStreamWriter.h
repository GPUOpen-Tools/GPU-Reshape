#pragma once

// Layer
#include <Backends/DX12/Compiler/DXStream.h>

// Std
#include <cstdint>
#include <algorithm>

// Common
#include <Common/Assert.h>

/// Bit stream reader
struct LLVMBitStreamWriter {
    template<typename T>
    using ChunkType = std::conditional_t<(sizeof(T) > 4), uint64_t, uint32_t>;

    LLVMBitStreamWriter(DXStream& stream) : stream(stream) {
        ptr = stream.NextWord64();
    }

    /// Add the DXIL header
    ///   ! Must be run before all other operations
    void AddHeaderValidation() {
        constexpr uint32_t kMagic =
            ('B' << 0) |
            ('C' << 8) |
            (0x0 << 16) |
            (0xC << 20) |
            (0xE << 24) |
            (0xD << 28);

        // Write header
        Variable(kMagic, 32);
    }

    /// Write a fixed type
    /// \tparam T type to be used
    /// \param value value to be written
    /// \param fixedWidth bit width of this type
    template<typename T>
    void Fixed(T value, uint8_t fixedWidth = sizeof(T) * 8) {
        ASSERT(fixedWidth <= 64, "Fixed width must be less or equal to 64 bits");
        Variable<T>(value, fixedWidth);
    }

    /// Consume a fixed enum type
    /// \tparam T type to be used, underlying type also used for expansion
    /// \param value value to be written
    /// \param fixedWidth bit width of this type
    template<typename T>
    void FixedEnum(T value, uint8_t fixedWidth = sizeof(T) * 8) {
        ASSERT(fixedWidth <= 64, "Enum width must be less or equal to 64 bits");
        Variable(static_cast<std::underlying_type_t<T>>(value), fixedWidth);
    }

    /// Write a variable width value
    ///   Given N bits, 0..N-1 contains accumulated chunk. Bit N-1 indicates next chunk.
    /// \tparam T type to be used
    /// \param value the value to be written
    /// \param bitWidth the bit width of each chunk
    template<typename T>
    void VBR(T value, uint8_t bitWidth) {
        using TChunk = ChunkType<T>;

        uint8_t chunkBitOffset = 1 << (bitWidth - 1);

        TChunk mask = chunkBitOffset - 1;

        do {
            TChunk masked = value & mask;

            // Next chunk?
            if (value > masked) {
                masked |= chunkBitOffset;
            }

            Variable(masked, bitWidth);

            value >>= bitWidth - 1;
        } while (value);
    }

    /// Encode a signed LLVM value
    static uint64_t EncodeSigned(int64_t value) {
        uint64_t shl = value << 1;
        return (value < 0) ? (shl | 0x1) : shl;
    }

    /// Align the stream to 32 bits
    void AlignDWord() {
        if (bitOffset % 32 == 0) {
            return;
        }

        // On dword boundary?
        if (bitOffset < 32) {
            bitOffset = 32;
        } else {
            ptr = stream.NextWord64();
            bitOffset = 0;
        }
    }

    /// Write a set of dwords
    ///  ! Must be aligned beforehand
    /// \param data dword data
    /// \param wordCount number of words
    void WriteDWord(const uint8_t* data, uint32_t wordCount) {
        const auto* wordData = reinterpret_cast<const uint32_t*>(data);

        // Must be aligned
        ASSERT(bitOffset % 32 == 0, "Unaligned dword write");

        // Any to write?
        if (!wordCount) {
            return;
        }

        // On dword boundary?
        if (bitOffset != 0) {
            // Write first word out
            *ptr |= static_cast<uint64_t>(wordData[0]) << 32u;
            ptr = stream.NextWord64();

            // Next!
            wordData++;
            wordCount--;
        }

        // Is the result going to be aligned?
        const bool isAligned = wordCount % 2 == 0;

        // Write all missing words
        stream.AppendData(wordData, wordCount * sizeof(uint32_t));

        // Set new state
        ptr = stream.GetMutableData<uint64_t>();
        bitOffset = isAligned ? 0 : 32;
    }

    /// Write a char6 value
    /// \param ch ansi char
    void Char6(char ch) {
        uint8_t encoded;

        // Encode it
        if (ch >= 'a' && ch <= 'z') {
            encoded = (ch - 'a') + 0;
        } else if (ch >= 'A' && ch <= 'Z') {
            encoded = (ch - 'A') + 26;
        } else if (ch >= '0' && ch <= '9') {
            encoded = (ch - '0') + 52;
        } else if (ch == '.') {
            encoded = 62;
        } else if (ch == '_') {
            encoded = 63;
        } else {
            ASSERT(false, "Invalid char6 encoding");
            encoded = 0;
        }

        // Write out encoded char
        Variable<uint8_t>(encoded, 6);
    }

    /// Write a variable width data type
    /// \tparam T type to be used
    /// \param value value to be written
    /// \param count bit count
    template<typename T>
    void Variable(const T& value, uint8_t count) {
        uint8_t available = 64u - bitOffset;

        if (count > available) {
            *ptr |= static_cast<uint64_t>(value) << bitOffset;
            ptr = stream.NextWord64();
            *ptr |= static_cast<uint64_t>(value) >> available;
            bitOffset = count - available;
        } else if (count == available) {
            *ptr |= static_cast<uint64_t>(value) << bitOffset;
            ptr = stream.NextWord64();
            bitOffset = 0;
        } else {
            *ptr |= static_cast<uint64_t>(value) << bitOffset;
            bitOffset += count;
        }
    }

    /// Close this writer
    void Close() {
        if (bitOffset == 0) {
            stream.Resize(stream.GetByteSize() - sizeof(uint64_t));
        }
    }

private:
    /// Underlying stream
    DXStream& stream;

    /// Current word64
    uint64_t* ptr{nullptr};

    /// Current bit offset
    uint8_t bitOffset{0};
};
