// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

#pragma once

// Layer
#include <Backends/DX12/DX12.h>
#include "DXILPhysicalBlock.h"
#include "DXILHeader.h"
#include "LLVM/LLVMHeader.h"
#include "LLVM/LLVMBlock.h"
#include "LLVM/LLVMBitStreamWriter.h"

// Common
#include <Common/Containers/TrivialStackVector.h>
#include <Common/Containers/LinearBlockAllocator.h>

// Std
#include <vector>
#include <string>
#include <unordered_map>

// Forward declarations
struct DXStream;
struct LLVMBitStreamReader;
struct LLVMBitStreamWriter;
struct LLVMBlockMetadata;

/// DXIL LLVM decoder
struct DXILPhysicalBlockScan {
public:
    DXILPhysicalBlockScan(const Allocators& allocators);
    ~DXILPhysicalBlockScan();

    /// Set the block filter
    /// \param shlBitMask
    void SetBlockFilter(uint64_t shlBitMask);

    /// Set debugging postfix
    /// \param postfix
    void SetDebugPostfix(const char* postfix) {
        debugPostfix = postfix;
    }

    /// Scan the DXIL bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Scan(const void* byteCode, uint64_t byteLength);

    /// Stitch the resulting stream
    /// \param out
    void Stitch(DXStream& out);

    /// Copy to a new table
    /// \param out the destination table
    void CopyTo(DXILPhysicalBlockScan& out);

    /// Get the root block
    /// \return
    const LLVMBlock& GetRoot() const {
        return root;
    }

    /// Get the root block
    /// \return
    LLVMBlock& GetRoot() {
        return root;
    }

private:
    /// Destroy a block
    /// \param block source block
    void DestroyBlockContents(const LLVMBlock* block);
    
    /// Copy a block
    /// \param block source block
    /// \param out destination block
    void CopyBlock(const LLVMBlock* block, LinearBlockAllocator<sizeof(uint64_t) * 1024u>& outRecordAllocator, LLVMBlock& out);

private:
    /// Result of a scanning operation
    enum class ScanResult {
        OK,
        Error,
        Stop
    };

    /// Scan a new sub block
    /// \param stream the current stream
    /// \param block sub block to populate
    /// \return success or traversal state
    ScanResult ScanEnterSubBlock(LLVMBitStreamReader& stream, LLVMBlock* block);

    /// Scan an abbreviation
    /// \param stream the current stream
    /// \param block block to populate
    /// \return success or traversal state
    ScanResult ScanAbbreviation(LLVMBitStreamReader& stream, LLVMBlock* block);

    /// Scan an unabbreviated record
    /// \param stream the current stream
    /// \param block block to populate
    /// \return success or traversal state
    ScanResult ScanUnabbreviatedRecord(LLVMBitStreamReader& stream, LLVMBlock* block);

    /// Scan an unabbreviated INFOBLOCK record
    /// \param stream the current stream
    /// \param block block to populate
    /// \return success or traversal state
    ScanResult ScanUnabbreviatedInfoRecord(LLVMBitStreamReader& stream, LLVMBlockMetadata** metadata);

    /// Scan a new sub BLOCKINFO
    /// \param stream the current stream
    /// \param block sub BLOCKINFO to populate
    /// \param abbreviationSize fixed abbreviation size for record scanning
    /// \return success or traversal state
    ScanResult ScanBlockInfo(LLVMBitStreamReader& stream, LLVMBlock* block, uint32_t abbreviationSize);

    /// Scan an abbreviated record
    /// \param stream the current stream
    /// \param block block to populate
    /// \param encodedAbbreviationId encoded identifier for abbreviation expansion
    /// \return success or traversal state
    ScanResult ScanRecord(LLVMBitStreamReader& stream, LLVMBlock* block, uint32_t encodedAbbreviationId);

    /// Expand a trivial abbreviated parameter
    /// \param stream the current stream
    /// \param parameter the trivial parameter
    /// \return expanded value
    uint64_t ScanTrivialAbbreviationParameter(LLVMBitStreamReader& stream, const LLVMAbbreviationParameter& parameter);

private:
    /// Result of a write operation
    enum class WriteResult {
        OK,
        Error
    };

    /// Write a sub block
    /// \param stream destination stream
    /// \param block source block
    /// \return success state
    WriteResult WriteSubBlock(LLVMBitStreamWriter& stream, const LLVMBlock* block);

    /// Write the BLOCKINFO
    /// \param stream destination stream
    /// \param block source block
    /// \return success state
    WriteResult WriteBlockInfo(LLVMBitStreamWriter &stream, const LLVMBlock *block, const LLVMBitStreamWriter::Position &lengthPos);

    /// Write an abbreviation
    /// \param stream destination stream
    /// \param abbreviation the abbreviation to be written
    /// \return success state
    WriteResult WriteAbbreviation(LLVMBitStreamWriter& stream, const LLVMAbbreviation& abbreviation);

    /// Write a record
    /// \param stream destination stream
    /// \param record the record to be written
    /// \return success state
    WriteResult WriteRecord(LLVMBitStreamWriter& stream, const LLVMBlock* block, const LLVMRecord& record);

    /// Write an unabbreviated record
    /// \param stream destination stream
    /// \param record the record to be written
    /// \return success state
    WriteResult WriteUnabbreviatedRecord(LLVMBitStreamWriter& stream, const LLVMRecord& record);

    /// Write a a trivial abbreviation parameter
    /// \param stream destination stream
    /// \param parameter the parameter to be written
    /// \param op the source opcode
    /// \return success state
    WriteResult WriteTrivialAbbreviationParameter(LLVMBitStreamWriter& stream, const LLVMAbbreviationParameter& parameter, uint64_t op);

private:
    /// Top header
    DXILHeader header;

    /// Hierarchical root block
    LLVMBlock root;

private:
    /// UID counter
    uint32_t uidCounter{0};

    /// Current filter
    uint64_t shlBlockFilter = UINT64_MAX;

    /// Debugging postfix
    const char* debugPostfix = "";

    /// Cache for flat operand scanning
    Vector<uint64_t> recordOperandCache;

    /// Lookup for out-of-place BLOCKINFO association
    std::unordered_map<uint32_t, LLVMBlockMetadata*> metadataLookup;
    
    /// Shared allocator for records
    LinearBlockAllocator<sizeof(uint64_t) * 1024u> recordAllocator;

private:
    Allocators allocators;
};
