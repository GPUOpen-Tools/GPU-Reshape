#pragma once

// Layer
#include <Backends/DX12/DX12.h>
#include "DXILPhysicalBlock.h"
#include "DXILHeader.h"
#include "LLVM/LLVMHeader.h"
#include "LLVM/LLVMBlock.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

// Std
#include <vector>
#include <string>
#include <map>

// Forward declarations
struct LLVMBitStream;
struct LLVMBlockMetadata;

/// DXIL LLVM decoder
struct DXILPhysicalBlockScan {
public:
    DXILPhysicalBlockScan(const Allocators& allocators);

    /// Scan the DXIL bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Scan(const void* byteCode, uint64_t byteLength);

    /// Get the root block
    /// \return
    const LLVMBlock& GetRoot() {
        return root;
    }

private:
    /// Read all module records
    /// \return
    bool ReadAndValidateModuleRecords();

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
    ScanResult ScanEnterSubBlock(LLVMBitStream& stream, LLVMBlock* block);

    /// Scan an abbreviation
    /// \param stream the current stream
    /// \param block block to populate
    /// \return success or traversal state
    ScanResult ScanAbbreviation(LLVMBitStream& stream, LLVMBlock* block);

    /// Scan an unabbreviated record
    /// \param stream the current stream
    /// \param block block to populate
    /// \return success or traversal state
    ScanResult ScanUnabbreviatedRecord(LLVMBitStream& stream, LLVMBlock* block);

    /// Scan an unabbreviated INFOBLOCK record
    /// \param stream the current stream
    /// \param block block to populate
    /// \return success or traversal state
    ScanResult ScanUnabbreviatedInfoRecord(LLVMBitStream& stream, LLVMBlockMetadata** metadata);

    /// Scan a new sub BLOCKINFO
    /// \param stream the current stream
    /// \param block sub BLOCKINFO to populate
    /// \param abbreviationSize fixed abbreviation size for record scanning
    /// \return success or traversal state
    ScanResult ScanBlockInfo(LLVMBitStream& stream, LLVMBlock* block, uint32_t abbreviationSize);

    /// Scan an abbreviated record
    /// \param stream the current stream
    /// \param block block to populate
    /// \param encodedAbbreviationId encoded identifier for abbreviation expansion
    /// \return success or traversal state
    ScanResult ScanRecord(LLVMBitStream& stream, LLVMBlock* block, uint32_t encodedAbbreviationId);

    /// Expand a trivial abbreviated parameter
    /// \param stream the current stream
    /// \param parameter the trivial parameter
    /// \return expanded value
    uint64_t ScanTrivialAbbreviationParameter(LLVMBitStream& stream, const LLVMAbbreviationParameter& parameter);

private:
    /// Top header
    DXILHeader header;

    /// Hierarchical root block
    LLVMBlock root;

private:
    /// Cache for flat operand scanning
    std::vector<uint64_t> recordOperandCache;

    /// Lookup for out-of-place BLOCKINFO association
    std::map<uint32_t, LLVMBlockMetadata*> metadataLookup;

private:
    Allocators allocators;
};
