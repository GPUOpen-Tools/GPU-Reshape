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

/// Qualifiers and helpers
#ifdef __cplusplus
#   define HLSL_REF(X) X&
#   define HLSL_MAX(A, B) std::max(A, B)
#   define HLSL_INLINE inline
#   define HLSL_CONSTEXPR static constexpr
#else // __cplusplus
#   define HLSL_REF(X) inout X
#   define HLSL_MAX(A, B) max(A, B)
#   define HLSL_INLINE 
#   define HLSL_CONSTEXPR static const
#endif // __cplusplus

/// HLSL side struct equivalents
#ifndef __cplusplus
struct D3D12_GPU_VIRTUAL_ADDRESS_RANGE {
    UInt64 StartAddress;
    UInt64 SizeInBytes;
};

struct D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE {
    UInt64 StartAddress;
    UInt64 SizeInBytes;
    UInt64 StrideInBytes;
};

struct D3D12_DISPATCH_RAYS_DESC {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE RayGenerationShaderRecord;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE MissShaderTable;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE HitGroupTable;
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE CallableShaderTable;
    uint Width;
    uint Height;
    uint Depth;
};

struct D3D12_DISPATCH_ARGUMENTS {
    uint ThreadGroupCountX;
    uint ThreadGroupCountY;
    uint ThreadGroupCountZ;
};

/// Constants
#define D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT  64
#define D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT 32
#define D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES         32
#endif // __cplusplus
