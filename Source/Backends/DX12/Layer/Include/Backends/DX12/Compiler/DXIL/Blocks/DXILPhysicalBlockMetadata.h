#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/DXILHeader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>
#include "DXILPhysicalBlockSection.h"

// Std
#include <string_view>
#include <vector>

/// Type block
struct DXILPhysicalBlockMetadata : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Parse all instructions
    void ParseMetadata(const struct LLVMBlock *block);

    /// Get the medata handle type
    /// \param handleID the unique handle id
    /// \return nullptr if not found
    const Backend::IL::Type* GetHandleType(uint32_t handleID);

private:
    /// Parse a named node successor
    /// \param block
    /// \param record
    /// \param name
    void ParseNamedNode(const struct LLVMBlock *block, const struct LLVMRecord& record, const struct LLVMRecordStringView& name);

    /// Parse a resource list
    /// \param block parent block
    /// \param type the class type
    /// \param id
    void ParseResourceList(const struct LLVMBlock *block, DXILShaderResourceClass type, uint32_t id);

    /// Get the IL component ype
    /// \param type dxil type
    /// \return type, optionally unexposed
    const Backend::IL::Type* GetComponentType(ComponentType type);

    /// Get the IL component format
    /// \param type dxil type
    /// \return format, optionally unexposed
    Backend::IL::Format GetComponentFormat(ComponentType type);

    /// Get an operand constant
    const IL::Constant* GetOperandConstant(uint32_t id) {
        ASSERT(id != 0, "Null metadata operand");
        return metadata[id - 1].value.constant;
    }

    /// Get an operand constant
    template<typename T>
    const T* GetOperandConstant(uint32_t id) {
        return GetOperandConstant(id)->Cast<T>();
    }

    /// Get an operand constant
    template<typename T = uint32_t>
    const T GetOperandU32Constant(uint32_t id) {
        return static_cast<T>(GetOperandConstant(id)->Cast<IL::IntConstant>()->value);
    }

private:
    /// All resource entries
    struct Resources {
        uint64_t srvs = ~0;
        uint64_t uavs = ~0;
        uint64_t cbvs = ~0;
        uint64_t samplers = ~0;
    } resources;

    struct HandleEntry {
        /// Class of parent list
        DXILShaderResourceClass _class;

        /// Source record
        const LLVMRecord* record{nullptr};

        /// Resource type
        const Backend::IL::Type* type{nullptr};
    };

    /// All handles
    std::vector<HandleEntry> handleEntries;

private:
    enum class MetadataType {
        None,
        Value
    };

    struct Metadata {
        /// Source record
        const LLVMRecord* record;

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

    /// All hosted metadata
    std::vector<Metadata> metadata;
};
