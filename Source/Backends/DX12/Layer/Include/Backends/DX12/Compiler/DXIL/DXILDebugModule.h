#pragma once

// Layer
#include "DXILPhysicalBlockScan.h"
#include <Backends/DX12/Compiler/IDXDebugModule.h>

// Common
#include <Common/Containers/LinearBlockAllocator.h>

// Std
#include <string>

struct DXILDebugModule final : public IDXDebugModule {
    DXILDebugModule(const Allocators &allocators);

    /// Parse the DXIL bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Parse(const void* byteCode, uint64_t byteLength);

    ///Overrides
    DXSourceAssociation GetSourceAssociation(uint32_t codeOffset) override;
    std::string_view GetLine(uint32_t fileUID, uint32_t line) override;
    std::string_view GetFilename() override;
    uint32_t GetFileCount() override;
    uint64_t GetCombinedSourceLength(uint32_t fileUID) const override;
    void FillCombinedSource(uint32_t fileUID, char *buffer) const override;

private:
    /// Parse all types
    /// \param block source block
    void ParseTypes(LLVMBlock* block);

    /// Parse a module specific function
    /// \param record source record
    void ParseModuleFunction(const LLVMRecord& record);

    /// Parse all constants
    /// \param block source block
    void ParseConstants(LLVMBlock* block);

    /// Parse all metadata
    /// \param block source block
    void ParseMetadata(LLVMBlock* block);

    /// Parse a named metadata node
    /// \param block source block
    /// \param record source record
    /// \param name given name of the node
    void ParseNamedMetadata(LLVMBlock* block, const LLVMRecord& record, const struct LLVMRecordStringView& name);

    /// Parse operand contents
    /// \param block source block
    /// \param record source record
    void ParseContents(LLVMBlock* block, const LLVMRecord& record);

    /// Parse a function
    /// \param block source block
    void ParseFunction(LLVMBlock* block);

private:
    /// Scanner
    DXILPhysicalBlockScan scan;

private:
    struct SourceFragment {
        /// Filename of this fragment
        std::string filename;

        /// Total contents of this fragment
        std::string contents;

        /// All summarized line offsets, including base (0) line
        std::vector<uint32_t> lineOffsets;
    };

    /// All source fragments within a module
    std::vector<SourceFragment> sourceFragments;

private:
    struct InstructionMetadata {
        /// Optional source association to the fragments
        DXSourceAssociation sourceAssociation;
    };

    /// All instruction data, used for cross referencing
    std::vector<InstructionMetadata> instructionMetadata;

private:
    /// Lightweight type definition
    struct ThinType {
        /// Underlying type
        LLVMTypeRecord type{LLVMTypeRecord::Void};

        /// Is this type non-semantic? Meaning, stripped from the canonical module?
        bool bIsNonSemantic{false};

        /// Payload data
        union {
            struct {
                bool isVoidReturn;
            } function;
        };
    };

    /// Lightweight value definition
    struct ThinValue {
        /// Optional type
        uint32_t thinType{~0u};

        /// Is this value non-semantic? Meaning, stripped from the canonical module?
        bool bIsNonSemantic{false};
    };

    /// All types
    std::vector<ThinType> thinTypes;

    /// All values
    std::vector<ThinValue> thinValues;
};
