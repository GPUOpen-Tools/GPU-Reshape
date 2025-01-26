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
#include <Backends/DX12/DX12.h>

// Common
#include <cstdint>

// Forward declarations
enum class DXBCRuntimeDataShaderKind : uint32_t;
enum class DXBCRuntimeDataSubObjectKind : uint32_t;

enum class DXBCType {
    None,
    Function,
    SubObject
};

struct DXBCSubObjectAssociationView {
    /// Get string
    const char* operator[](uint32_t i) const {
        ASSERT(i < indexCount, "Out of bounds");
        return stringStart + indices[i];
    }

    /// String buffer start
    const char* stringStart;

    /// All indices, does not include RDAT count start
    const uint32_t* indices;

    /// Number of indices
    uint32_t indexCount;
};

struct DXBCSubObjectExport {
    /// Type of this sub-object
    DXBCRuntimeDataSubObjectKind subObjectKind;

    /// General payload
    union {
        D3D12_STATE_OBJECT_CONFIG config;
        
        D3D12_RAYTRACING_SHADER_CONFIG shaderConfig;
        
        D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
        
        D3D12_RAYTRACING_PIPELINE_CONFIG1 pipelineConfig1;

        struct {
            const void* data;
            uint32_t length;
        } rootSignature;

        struct {
            const char* subObject;
            DXBCSubObjectAssociationView exportView;
        } subObjectToExportsAssociation;

        struct {
            uint32_t type;
            const char* anyHitExport;
            const char* closestHitExport;
            const char* intersectionExport;
        } hitGroup;
    };
};

struct DXBCFunctionExport {
    /// Lifetime is tied to the DXBC blob
    const char* mangledName;
            
    /// Kind of the export
    DXBCRuntimeDataShaderKind kind;
};

struct DXBCExport {
    /// Lifetime is tied to the DXBC blob
    const char* unmangledName{nullptr};

    /// Type of this export
    DXBCType type{DXBCType::None};

    /// General payload
    union {
        DXBCFunctionExport function;
        DXBCSubObjectExport subObject;
    };
};
