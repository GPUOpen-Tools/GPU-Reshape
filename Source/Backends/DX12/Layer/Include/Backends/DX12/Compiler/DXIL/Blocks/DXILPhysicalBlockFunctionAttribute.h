#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/DXILFunctionDeclaration.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordReader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMRecordStringView.h>
#include "DXILPhysicalBlockSection.h"

// Common
#include <Common/Containers/TrivialStackVector.h>

/// Function block
struct DXILPhysicalBlockFunctionAttribute : public DXILPhysicalBlockSection {
    using DXILPhysicalBlockSection::DXILPhysicalBlockSection;

    /// Copy this block
    /// \param out destination block
    void CopyTo(DXILPhysicalBlockFunctionAttribute& out);

    /// Set the declaration block
    /// \param block program block
    void SetDeclarationBlock(struct LLVMBlock *block);

public:
    /// Parse a function
    /// \param block source block
    void ParseParameterAttributeGroup(struct LLVMBlock *block);

    /// Parse a function
    /// \param block source block
    void ParseParameterBlock(struct LLVMBlock *block);

public:
    /// Get an attribute list
    /// \param count the number of attributes
    /// \param values all attribute values
    /// \return index
    uint32_t FindOrCompileAttributeList(uint32_t count, const LLVMParameterGroupValue* values);

private:
    struct ParameterAttribute {
        /// Parameter index
        uint32_t idx{~0u};

        /// Givne value
        LLVMParameterGroupValue value{LLVMParameterGroupValue::None};

        /// Optional string view
        LLVMRecordStringView view;
    };

    struct ParameterAttributeGroup {
        TrivialStackVector<ParameterAttribute, 16> attributes;
    };

    struct ParameterGroup {
        TrivialStackVector<ParameterAttribute, 16> attributes;
    };

    /// All attribute groups
    TrivialStackVector<ParameterAttributeGroup, 16> parameterAttributeGroups;

    /// All parameter groups
    TrivialStackVector<ParameterGroup, 16> parameterGroups;

private:
    /// Declaration blocks
    LLVMBlock* groupDeclarationBlock{nullptr};
    LLVMBlock* parameterDeclarationBlock{nullptr};
};
