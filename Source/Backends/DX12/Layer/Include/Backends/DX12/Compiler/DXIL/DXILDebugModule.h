// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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

// Std
#include <unordered_map>
#include <string>

// Forward declarations
class DXILModule;
struct DXBCPhysicalBlockShaderSourceInfo;

struct DXILDebugModule final : public IDXDebugModule {
    DXILDebugModule(const Allocators &allocators, DXILModule* module, const DXBCPhysicalBlockShaderSourceInfo& shaderSourceInfo);

    /// Parse the DXIL bytecode
    /// \param byteCode code start
    /// \param byteLength byte size of code
    /// \return success state
    bool Parse(const void* byteCode, uint64_t byteLength);

    ///Overrides
    DXSourceAssociation GetSourceAssociation(const IL::Function* function, uint32_t codeOffset) override;
    std::span<DXInstructionAssociation> GetInstructionAssociations(uint16_t fileUID, uint32_t line) override;
    DXDwarfInfo GetDwarfInfo(Backend::IL::TypeMap& typeMap, const IL::Function* function, uint32_t codeOffset) override;
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

    /// Parse the symbol table
    /// @param child source block
    void ParseSymTab(LLVMBlock * child);
    
    /// Parse a named metadata node
    /// \param block source block
    /// \param record source record
    /// \param name given name of the node
    void ParseNamedMetadata(LLVMBlock* block, uint32_t anchor, const LLVMRecord& record, const struct LLVMRecordStringView& name);

    /// Parse operand contents
    /// \param block source block
    /// \param fileMdId file id
    void ParseContentsRecord(LLVMBlock* block, uint32_t fileMdId);
    
    /// Parse operand contents
    /// \param filename base filename
    /// \param contents combined contents
    template<typename T>
    void ParseContentsAdapter(const T& filename, const T& contents);

    /// Parse a function
    /// \param block source block
    void ParseFunction(LLVMBlock* block);

    /// Remap all line scopes for unresolved metadata
    void RemapLineScopes();

    /// Create all reverse associations
    void CreateReverseAssociations();
    
    /// Get the linear file index
    /// \param scopeMdId scope id
    uint32_t GetLinearFileUID(uint32_t scopeMdId);

private:
    /// Create source fragments from the optional source block
    void CreateFragmentsFromSourceBlock();

private:
    /// Get a value cstring mapping, may incur an allocation
    /// \param id value id
    /// \return nullptr if not found
    const char* GetValueAllocation(uint32_t id);
    
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
    SourceFragment* FindOrCreateSourceFragmentSanitized(const LLVMRecordStringView& view);

    /// Find or create a source fragment
    /// \param view filename view
    SourceFragment* FindOrCreateSourceFragmentSanitized(const std::string_view& view);

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

    struct InstructionDwarfValue {
        /// Type of this value
        LLVMDwarfOpKind kind;

        /// Assigned value id
        uint32_t valueId{0};

        /// Owning value
        DXDwarfCode code;

        /// Payload
        union {
            struct {
                uint32_t bitStart;
                uint32_t bitLength;
            } bitWise;
        };
    };

    struct InstructionDwarfVariable {
        /// Name of the written variable
        const char* name {nullptr};

        /// Variable being assigned
        uint32_t variableMdId{0};

        /// Metadata type id
        uint32_t typeMdId{0};

        /// All values assigned to this instruction
        std::vector<InstructionDwarfValue*> values;
    };

    struct InstructionDwarfInfo {
        /// All variables assigned to this instruction
        std::vector<InstructionDwarfVariable> variables;
    };

    struct FunctionMetadata {
        /// All instruction data, used for cross referencing
        std::vector<InstructionMetadata> instructionMetadata;

        /// Code offset to dwarf info
        std::unordered_map<uint32_t, InstructionDwarfInfo> instructionDwarfInfos;
    };

    struct InstructionAssociationSet {
        /// All instructions that are associated to this location
        std::vector<DXInstructionAssociation> set;
    };

    /// All function metadata, ordered by link index
    Vector<FunctionMetadata> functionMetadata;

    /// Reverse instruction associations
    std::unordered_map<uint64_t, InstructionAssociationSet> instructionAssociations;

    /// Unresolved debug values
    Vector<InstructionDwarfValue*> unresolvedDwarfValues;

private:
    /// Parse a special debug call
    void ParseDebugCall(FunctionMetadata& functionMd, const LLVMRecord &record, uint32_t anchor, uint32_t functionValueIndex);

    /// Parse a special debug value call
    void ParseDebugValueCall(FunctionMetadata& functionMd, const LLVMRecord &record, uint32_t anchor);

    /// Resolve a forward dwarf value
    void ResolveDwarfValue(InstructionDwarfValue* value);
    
    /// Allocate a thin constant
    /// \param decl Constant declaration 
    template<class T>
    const T *AllocateThinConstant(const T &decl);

    /// Resolve a full constant from its id
    const IL::Constant* ResolveConstant(uint32_t valueId);
    
private:
    uint64_t GetLiteralConstant(uint32_t valueId);
    
private:
    /// Symtab values
    Vector<LLVMRecordStringView> valueStrings;

    /// String values, allocated on demand
    Vector<char*> valueAllocations;
    
private:
    struct Metadata {
        /// Underlying MD
        LLVMMetadataRecord type{};

        /// Owning record
        const LLVMRecord *record{nullptr};

        /// Payload data
        union {
            uint32_t value;
            
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

            struct {
                LLVMDwarfOpKind op;
                uint32_t nameMdId;
                uint32_t mdTypeId;
            } localVar;

            struct {
                LLVMDwarfOpKind op;
                union {
                    struct {
                        uint32_t bitStart;
                        uint32_t bitLength;
                    } bitPiece;
                };
            } expression;

            struct {
                LLVMDwarfTag tag;
                union {
                    struct {
                        uint32_t nameMdId;
                        uint32_t baseTypeMdId;
                    } _typedef;

                    struct {
                        uint32_t nameMdId;
                        uint32_t baseTypeMdId;
                        uint32_t size;
                        uint32_t align;
                        uint32_t offset;
                    } member;
                    
                    struct {
                        uint32_t baseTypeMdId;
                    } _const;
                };
            } derivedType;

            struct {
                LLVMDwarfTag tag;
                union {
                    struct {
                        uint32_t nameMdId;
                        uint32_t size;
                        uint32_t align;
                        uint32_t elementsMdId;
                        uint32_t templateParamsMdId;
                    } _class;
                    
                    struct {
                        uint32_t nameMdId;
                        uint32_t size;
                        uint32_t align;
                        uint32_t elementsMdId;
                    } structureType;
                    
                    struct {
                        uint32_t baseTypeMdId;
                        uint32_t size;
                        uint32_t align;
                        uint32_t elementsMdId;
                    } arrayType;
                };
            } compositeType;
            
            struct {
                uint32_t count;
                uint32_t lowerBound;
            } subRange;

            struct {
                uint32_t nameMdId;
                uint32_t typeMdId;
            } templateType;

            struct {
                uint32_t nameMdId;
                uint32_t typeMdId;
                uint32_t valueMdId;
            } templateValue;

            struct {
                uint32_t nameMdId;
                uint32_t size;
                uint32_t align;
                LLVMDwarfTypeEncoding encoding;
            } basicType;
        };
    };

    /// All metadata
    Vector<Metadata> thinMetadata;

private:
    /// Get the backend type from a dward type
    /// @param id dwarf id
    /// @return type
    const Backend::IL::Type* GetTypeFromDwarf(Backend::IL::TypeMap& typeMap, uint32_t id);

    /// Get the class backend type from a dward type
    /// @param typeMd class md
    /// @return type
    const Backend::IL::Type* GetClassTypeFromDwarf(Backend::IL::TypeMap& typeMap, const Metadata& typeMd);

    /// Get the struct backend type from a dward type
    /// @param typeMd class md
    /// @return type
    const Backend::IL::Type* GetStructureTypeFromDwarf(Backend::IL::TypeMap& typeMap, const Metadata& typeMd);

    /// Get the array backend type from a dward type
    /// @param typeMd array md
    /// @return type
    const Backend::IL::Type* GetArrayTypeFromDwarf(Backend::IL::TypeMap& typeMap, const Metadata& typeMd);
    
    /// Get the basic backend type from a dward type
    /// @param typeMd basic md
    /// @return type
    const Backend::IL::Type* GetBasicTypeFromDwarf(Backend::IL::TypeMap& typeMap, const Metadata& typeMd);

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
                uint32_t* parameterTypes;
                uint32_t  parameterCount : 16;
                uint32_t  isVoidReturn   : 1;
            } function;
            
            struct {
                uint8_t bitWidth;
            } integral;
            
            struct {
                uint8_t contained;
            } aggregate;
        };
    };

    /// Value validation kind
    enum class ThinValueKind {
        None,
        Constant,
        Function,
        Parameter,
        Instruction
    };

    /// Lightweight value definition
    struct ThinValue {
        /// Kind of this value
        ThinValueKind kind{ThinValueKind::None};
        
        /// Optional type
        uint32_t thinType{~0u};

        /// Offset of the declaring record
        uint32_t recordOffset{0};

        /// Constant owning this value
        LLVMRecord* record{nullptr};

        /// Is this value non-semantic? Meaning, stripped from the canonical module?
        bool bIsNonSemantic{false};
    };

    /// All types
    Vector<ThinType> thinTypes;

    /// All values
    Vector<ThinValue> thinValues;

    /// Allocator for misc data
    LinearBlockAllocator<256> blockAllocator;

private:
    struct ThinFunction {
        uint32_t thinType{~0u};
    };

    /// All functions, appear in linkage order
    Vector<ThinFunction> thinFunctions;

    /// Current linking index
    uint32_t functionLinkIndex{0};

private:
    Allocators allocators;

    /// Source module
    DXILModule* module{nullptr};

    /// Optional, source info block
    const DXBCPhysicalBlockShaderSourceInfo& shaderSourceInfo;
};
