#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/DXILHeader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>
#include "DXILPhysicalBlockSection.h"

// Std
#include <string_view>
#include <vector>

// Forward declarations
struct DXJob;

/// Type block
struct DXILPhysicalBlockMetadata : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

public:
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

public:
    /// Compile all instructions
    void CompileMetadata(struct LLVMBlock *block);

    /// Stitch all instructions
    void StitchMetadata(struct LLVMBlock *block);

public:
    /// Compile the export resource metadata
    void CompileShaderExportResources(const DXJob& job);

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
        /// Owning uid
        uint32_t uid{~0u};

        /// All hosted metadata
        std::vector<Metadata> metadata;
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

    /// Get the IL component ype
    /// \param type dxil type
    /// \return type, optionally unexposed
    const Backend::IL::Type* GetComponentType(ComponentType type);

    /// Get the IL component format
    /// \param type dxil type
    /// \return format, optionally unexposed
    Backend::IL::Format GetComponentFormat(ComponentType type);

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

private:
    /// All resource entries
    struct Resources {
        uint32_t uid = ~0u;
        uint32_t source = ~0u;
        uint64_t srvs = ~0;
        uint64_t uavs = ~0;
        uint64_t cbvs = ~0;
        uint64_t samplers = ~0;
    } resources;

    /// Represents a handle within a space
    struct HandleEntry {
        /// Source record
        const LLVMRecord* record{nullptr};

        /// Resource type
        const Backend::IL::Type* type{nullptr};

        /// Bind space
        uint32_t registerBase{~0u};
        uint32_t registerRange{~0u};
        uint32_t bindSpace{~0u};
    };

    /// A mapped register class
    struct MappedRegisterClass {
        /// Class of this space
        DXILShaderResourceClass _class;

        /// All handles within this space
        std::vector<uint32_t> handles;

        /// All handles within this space
        std::vector<uint32_t> resourceLookup;
    };

    /// A user register space
    struct UserRegisterSpace {
        /// Space index
        uint32_t space{~0u};

        /// All handles within this space
        std::vector<uint32_t> handles;

        /// Current register bound
        uint32_t registerBound{0};
    };

    /// All handles
    std::vector<MappedRegisterClass> registerClasses;

    /// All handles
    std::vector<UserRegisterSpace> registerSpaces;

    /// All handles within this space
    std::vector<HandleEntry> handles;

    /// Current register space bound
    uint32_t registerSpaceBound{0};

    /// All hosted metadata blocks
    std::vector<MetadataBlock> metadataBlocks;

private:
    /// Find a register class, allocate if missing
    /// \param _class designated class
    /// \return register class
    MappedRegisterClass& FindOrAddRegisterClass(DXILShaderResourceClass _class);

    /// Find a register space, allocate if missing
    /// \param space designated space
    /// \return register space
    UserRegisterSpace& FindOrAddRegisterSpace(uint32_t space);

private:
    /// Internal handle id
    uint32_t exportHandleId{ 0 };

private:
    /// Declaration blocks
    LLVMBlock* declarationBlock{nullptr};
};
