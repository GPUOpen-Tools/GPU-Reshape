#pragma once

// Layer
#include <Backends/DX12/Compiler/DXIL/DXILHeader.h>
#include <Backends/DX12/Compiler/DXIL/LLVM/LLVMBlock.h>

// Backend
#include <Backend/IL/Type.h>

/// Represents a handle
struct DXILMetadataHandleEntry {
    /// Source record
    const LLVMRecord* record{nullptr};

    /// Resource type
    const Backend::IL::Type* type{nullptr};

    /// Binding class
    DXILShaderResourceClass _class{};

    /// Binding register base
    uint32_t registerBase{~0u};

    /// Binding register range
    uint32_t registerRange{~0u};

    /// Binding space
    uint32_t bindSpace{~0u};

    /// Metadata name
    const char* name{""};

    /// Class specific data
    union {
        /// Unordered metadata
        struct {
            /// Underlying component type
            ComponentType componentType;

            /// Underlying shape
            DXILShaderResourceShape shape;
        } uav;

        /// Resource metadata
        struct {
            /// Underlying component type
            ComponentType componentType;
            
            /// Underlying shape
            DXILShaderResourceShape shape;
        } srv;
    };
};
