// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
    std::string_view GetSourceFilename(uint32_t fileUID) override;
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
    void ParseNamedMetadata(LLVMBlock* block, uint32_t anchor, const LLVMRecord& record, const struct LLVMRecordStringView& name);

    /// Parse operand contents
    /// \param block source block
    /// \param fileMdId file id
    void ParseContents(LLVMBlock* block, uint32_t fileMdId);

    /// Parse a function
    /// \param block source block
    void ParseFunction(LLVMBlock* block);

    /// Remap all line scopes for unresolved metadata
    void RemapLineScopes();

    /// Get the linear file index
    /// \param scopeMdId scope id
    uint32_t GetLinearFileUID(uint32_t scopeMdId);

private:
    /// Scanner
    DXILPhysicalBlockScan scan;

private:
    struct SourceFragmentDirective {
        /// File identifier
        uint16_t fileUID{UINT16_MAX};

        /// Line offset within the target file
        uint32_t fileLineOffset{0};

        /// Line offset in the target fragment
        uint32_t directiveLineOffset{0};
    };
    
    struct SourceFragment {
        SourceFragment(const Allocators& allocators) : lineOffsets(allocators), preprocessedDirectives(allocators) {
            /** */
        }
        
        /// Filename of this fragment
        std::string filename;

        /// Total contents of this fragment
        std::string contents;

        /// Identifier of this file
        uint16_t uid{0};

        /// All summarized line offsets, including base (0) line
        Vector<uint32_t> lineOffsets;

        /// All preprocessed fragments within this, f.x. files on line directives
        Vector<SourceFragmentDirective> preprocessedDirectives;
    };

    /// Is the contents considered unresolved? f.x. may happen with already preprocessed files
    bool isContentsUnresolved{false};

    /// Find or create a source fragment
    /// \param view filename view
    SourceFragment* FindOrCreateSourceFragment(const LLVMRecordStringView& view);

    /// Find or create a source fragment
    /// \param view filename view
    SourceFragment* FindOrCreateSourceFragment(const std::string_view& view);

    /// All source fragments within a module
    Vector<SourceFragment> sourceFragments;

private:
    struct InstructionMetadata {
        /// Optional source association to the fragments
        DXSourceAssociation sourceAssociation;
    };

    /// All instruction data, used for cross referencing
    Vector<InstructionMetadata> instructionMetadata;

private:
    struct Metadata {
        /// Underlying MD
        LLVMMetadataRecord type{};

        /// Payload data
        union {
            struct {
                uint32_t linearFileUID;
            } file;

            struct {
                uint32_t fileMdId;
            } lexicalBlock;

            struct {
                uint32_t fileMdId;
            } lexicalBlockFile;

            struct {
                uint32_t fileMdId;
            } subProgram;

            struct {
                uint32_t fileMdId;
            } _namespace;

            struct {
                uint32_t fileMdId;
            } compileUnit;
        };
    };

    /// All metadata
    Vector<Metadata> metadata;

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
    Vector<ThinType> thinTypes;

    /// All values
    Vector<ThinValue> thinValues;

private:
    Allocators allocators;
};
