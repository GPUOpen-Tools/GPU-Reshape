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
#include <Backends/DX12/Compiler/DXIL/DXILHeader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordView.h>
#include <Backends/DX12/Compiler/DXIL/Blocks/DXILPhysicalBlockSection.h>
#include <Backends/DX12/Compiler/DXIL/Blocks/DXILMetadataHandleEntry.h>

// Std
#include <string_view>
#include <vector>

// Forward declarations
struct DXCompileJob;

/// Type block
struct DXILPhysicalBlockMetadata : public DXILPhysicalBlockSection {
public:
    DXILPhysicalBlockMetadata(const Allocators &allocators, Backend::IL::Program &program, DXILPhysicalBlockTable &table);

    /// Copy this block
    /// \param out destination block
    void CopyTo(DXILPhysicalBlockMetadata& out);

    /// Set the declaration block
    /// \param block program block
    void SetDeclarationBlock(struct LLVMBlock *block);

public:
    /// Parse all instructions
    void ParseMetadata(const struct LLVMBlock *block);

    /// Get the medata handle type
    /// \param handleID the unique handle id
    /// \return nullptr if not found
    const Backend::IL::Type* GetHandleType(DXILShaderResourceClass _class, uint32_t handleID);

    /// Get the binding group that a handle type belongs to
    /// \param type type to check
    /// \return binding group
    IL::ID GetTypeSymbolicBindingGroup(const Backend::IL::Type* type);

public:
    /// Compile all records
    void CompileMetadata(struct LLVMBlock *block);

    /// Compile global metadata
    void CompileMetadata(const DXCompileJob& job);

    /// Stitch all records
    void StitchMetadata(struct LLVMBlock *block);

    /// Stitch all metadata records
    void StitchMetadataAttachments(struct LLVMBlock *block, const TrivialStackVector<uint32_t, 512>& recordRelocation);

public:
    /// Ensure this program supports UAV operations
    void EnsureUAVCapability();

    /// Add a new program shader flag
    void AddProgramFlag(DXILProgramShaderFlagSet flags);

    /// Create all resource handles
    /// \param job job to be compiled against
    void CreateResourceHandles(const DXCompileJob& job);

public:
    /// Get the IL component format
    /// \param type dxil type
    /// \return format, optionally unexposed
    Backend::IL::Format GetComponentFormat(ComponentType type);

    /// Get the component type
    /// \param format the IL format
    /// \return component type
    ComponentType GetFormatComponent(Backend::IL::Format format);
    
    /// Get the IL component ype
    /// \param type dxil type
    /// \return type, optionally unexposed
    const Backend::IL::Type* GetComponentType(ComponentType type);

private:
    void CompileProgramEntryPoints();

private:
    enum class MetadataType {
        None,
        Value
    };

    struct Metadata {
        /// Source record
        uint32_t source{~0u};

        /// Payload
        union {
            struct {
                const Backend::IL::Type* type;
                const IL::Constant* constant;
            } value;
        };

        /// Name associated
        std::string name;
    };

    struct MetadataBlock {
        MetadataBlock(const Allocators& allocators) : metadata(allocators), sourceMappings(allocators) {
            /** */
        }
        
        /// Owning uid
        uint32_t uid{~0u};

        /// All hosted metadata
        Vector<Metadata> metadata;

        /// All resolved source mappings
        Vector<uint64_t> sourceMappings;
    };

private:
    /// Parse a named node successor
    /// \param block
    /// \param record
    /// \param name
    void ParseNamedNode(MetadataBlock& metadataBlock, const struct LLVMBlock *block, const struct LLVMRecord& record, const struct LLVMRecordStringView& name, uint32_t index);

    /// Parse a resource list
    /// \param block parent block
    /// \param type the class type
    /// \param id
    void ParseResourceList(MetadataBlock& metadataBlock, const struct LLVMBlock *block, DXILShaderResourceClass type, uint32_t id);

    /// Get an operand constant
    const IL::Constant* GetOperandConstant(MetadataBlock& block, uint32_t id) {
        ASSERT(id != 0, "Null metadata operand");
        return block.metadata[id - 1].value.constant;
    }

    /// Get an operand constant
    template<typename T>
    const T* GetOperandConstant(MetadataBlock& block, uint32_t id) {
        return GetOperandConstant(block, id)->Cast<T>();
    }

    /// Get an operand constant
    template<typename T = uint32_t>
    const T GetOperandU32Constant(MetadataBlock& block, uint32_t id) {
        return static_cast<T>(GetOperandConstant(block, id)->Cast<IL::IntConstant>()->value);
    }

    /// Get an operand constant
    bool GetOperandBoolConstant(MetadataBlock& block, uint32_t id) {
        return GetOperandConstant(block, id)->Cast<IL::BoolConstant>()->value;
    }

private:
    /// Find or add a new string
    /// \param metadata destination md
    /// \param block destination block
    /// \param str string to be added
    /// \return md index
    uint32_t FindOrAddString(MetadataBlock& metadata, LLVMBlock* block, const std::string_view& str);

    /// Find or add a new constant
    /// \param metadata destination md
    /// \param block destination block
    /// \param constant constant to be added
    /// \return md index
    uint32_t FindOrAddOperandConstant(MetadataBlock& metadata, LLVMBlock* block, const Backend::IL::Constant* constant);

    /// Find or add a new u32 constant
    /// \param metadata destination md
    /// \param block destination block
    /// \param constant constant to be added
    /// \return md index
    uint32_t FindOrAddOperandU32Constant(MetadataBlock& metadata, LLVMBlock* block, uint32_t value) {
        return FindOrAddOperandConstant(metadata, block, program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::IntType{.bitWidth = 32, .signedness = true}),
            Backend::IL::IntConstant{.value = value}
        ));
    }

    /// Find or add a boolean constant
    /// \param metadata destination md
    /// \param block destination block
    /// \param constant constant to be added
    /// \return md index
    uint32_t FindOrAddOperandBoolConstant(MetadataBlock& metadata, LLVMBlock* block, bool value) {
        return FindOrAddOperandConstant(metadata, block, program.GetConstants().FindConstantOrAdd(
            program.GetTypeMap().FindTypeOrAdd(Backend::IL::BoolType{}),
            Backend::IL::BoolConstant{.value = value}
        ));
    }

private:
    /// Get a metadata block from id
    /// \param uid
    /// \return nullptr if not found
    MetadataBlock* GetMetadataBlock(uint32_t uid) {
        for (MetadataBlock& block : metadataBlocks) {
            if (block.uid == uid) {
                return &block;
            }
        }

        return nullptr;
    }

public:
    /// Entrypoint
    struct EntryPoint {
        uint32_t uid = ~0u;
        uint32_t program = ~0u;
    } entryPoint;

    /// All resource entries
    struct Resources {
        uint32_t uid = ~0u;
        uint32_t source = ~0u;
        uint32_t lists[static_cast<uint32_t>(DXILShaderResourceClass::Count)];
    } resources;

    /// A mapped register class
    struct MappedRegisterClass {
        MappedRegisterClass(const Allocators& allocators) : handles(allocators), resourceLookup(allocators) {
            /** */
        }
        
        /// Class of this space
        DXILShaderResourceClass _class;

        /// All handles within this space
        Vector<uint32_t> handles;

        /// All handles within this space
        Vector<uint32_t> resourceLookup;
    };

    /// A user register space
    struct UserRegisterSpace {
        UserRegisterSpace(const Allocators& allocators) : handles(allocators) {
            /** */
        }
        
        /// Space index
        uint32_t space{~0u};

        /// All handles within this space
        Vector<uint32_t> handles;

        /// Current register bound
        uint32_t registerBound{0};
    };

    /// Get the medata handle type
    /// \param _class resource class
    /// \param handleID the unique handle id
    /// \return nullptr if not found
    const DXILMetadataHandleEntry* GetHandle(DXILShaderResourceClass _class, uint32_t handleID);
    
    /// Get the medata handle type
    /// \param _class resource class
    /// \param handleID the unique handle id
    /// \return nullptr if not found
    const DXILMetadataHandleEntry* GetHandleFromMetadata(DXILShaderResourceClass _class, uint32_t handleID);

    /// Get the medata handle type
    /// \param _class resource class
    /// \param space binding space
    /// \param rangeLowerBound binding lower bound
    /// \param rangeUpperBound binding upper bound
    /// \return nullptr if not found
    const DXILMetadataHandleEntry* GetHandle(DXILShaderResourceClass _class, int64_t space, int64_t rangeLowerBound, int64_t rangeUpperBound);

    /// Get the symbolic texture bindings, always valid
    IL::ID GetSymbolicTextureBindings() const {
        return symbolicTextureBindings;
    }
    
    /// Get the symbolic sampler bindings, always valid
    IL::ID GetSymbolicSamplerBindings() const {
        return symbolicSamplerBindings;
    }
    
    /// Get the symbolic buffer bindings, always valid
    IL::ID GetSymbolicBufferBindings() const {
        return symbolicBufferBindings;
    }
    
    /// Get the symbolic buffer bindings, always valid
    IL::ID GetSymbolicCBufferBindings() const {
        return symbolicCBufferBindings;
    }

private:
    /// All handles
    Vector<MappedRegisterClass> registerClasses;

    /// All handles
    Vector<UserRegisterSpace> registerSpaces;

    /// All handles within this space
    Vector<DXILMetadataHandleEntry> handles;

    /// Current register space bound
    uint32_t registerSpaceBound{0};

    /// All hosted metadata blocks
    Vector<MetadataBlock> metadataBlocks;

private:
    /// Symbolic bindings
    IL::ID symbolicTextureBindings = IL::InvalidID;
    IL::ID symbolicSamplerBindings = IL::InvalidID;
    IL::ID symbolicBufferBindings  = IL::InvalidID;
    IL::ID symbolicCBufferBindings = IL::InvalidID;

private:
    /// Create all symbolic bindings
    void CreateSymbolicBindings();
    
    /// Find a register class, allocate if missing
    /// \param _class designated class
    /// \return register class
    MappedRegisterClass& FindOrAddRegisterClass(DXILShaderResourceClass _class);

    /// Find a register space, allocate if missing
    /// \param space designated space
    /// \return register space
    UserRegisterSpace& FindOrAddRegisterSpace(uint32_t space);

public:
    /// Compile the export resource metadata
    void EnsureProgramResourceClassList(const DXCompileJob& job);

    /// Compile the shader export handles
    void CreateShaderExportHandle(const DXCompileJob& job);

    /// Compile the PRMT handles
    void CreatePRMTHandle(const DXCompileJob& job);

    /// Compile the shader data handles
    void CreateShaderDataHandles(const DXCompileJob& job);

    /// Compile the descriptor data handles
    void CreateDescriptorHandle(const DXCompileJob& job);

    /// Compile the event handles
    void CreateEventHandle(const DXCompileJob& job);

    /// Compile the event handles
    void CreateConstantsHandle(const DXCompileJob& job);

    /// Compile class record metadata
    LLVMRecordView CompileResourceClassRecord(const MappedRegisterClass& mapped);

    /// Compile UAV metadata
    void CompileUAVResourceClass(const DXCompileJob& job);

    /// Compile SRV metadata
    void CompileSRVResourceClass(const DXCompileJob& job);

    /// Compile CBV metadata
    void CompileCBVResourceClass(const DXCompileJob& job);

    /// Compile all flags
    void CompileProgramFlags(const DXCompileJob& job);

    /// Check if a shading model has been satisfied
    bool SatisfiesShadingModel(uint32_t major, uint32_t minor) {
        return shadingModel.major > major || (shadingModel.major == major && shadingModel.minor >= minor); 
    }

    struct ShadingModel {
        DXILShadingModelClass _class;
        uint32_t major{1};
        uint32_t minor{0};
    } shadingModel;

    struct ValidationVersion {
        uint32_t major{1};
        uint32_t minor{0};
    } validationVersion;

    struct ProgramMetadata {
        /// User provided flags
        DXILProgramShaderFlagSet shaderFlags{ 0 };

        /// Internal shader flags
        DXILProgramShaderFlagSet internalShaderFlags{ 0 };
    } programMetadata;

    IL::ID entryPointId{IL::InvalidID};

private:
    /// Declaration blocks
    LLVMBlock* declarationBlock{nullptr};
};
