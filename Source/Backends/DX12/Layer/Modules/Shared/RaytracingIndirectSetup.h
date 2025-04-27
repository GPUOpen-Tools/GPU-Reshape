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

// Shared
#include "Cxx.h"
#include "ShaderRecordPatching.h"

/// Command constants
static const uint RaytracingIndirectSetupCommandByteStride   = 64;
static const uint RaytracingIndirectSetupCommandDWordStride  = RaytracingIndirectSetupCommandByteStride / sizeof(uint);
static const uint RaytracingIndirectSetupCommandRangeStride  = 5;
static const uint RaytracingIndirectSetupCommandRangeCount   = 4;

struct SBTIndirectSetupConstantData {
    uint ScratchByteCount;
    uint ConstantStride;
    uint CommandIndex;
    uint Pad1;
    
    UInt64 ScratchBaseAddress;
    UInt64 ConstantBaseAddress;

    SBTPatchConstantData CommonPatchConstants;
};

struct SBTSharedAllocationContext {
    /// The patched dispatch
    D3D12_DISPATCH_RAYS_DESC Dispatch;

    /// The descriptor lengths
    UInt64 RayGenDescriptorLength;
    UInt64 CallableDescriptorLength;
    UInt64 HitDescriptorLength;
    UInt64 MissDescriptorLength;

    /// The descriptor offsets
    UInt64 RayGenDescriptorOffset;
    UInt64 CallableDescriptorOffset;
    UInt64 HitDescriptorOffset;
    UInt64 MissDescriptorOffset;

    /// Total allocation size
    UInt64 AllocationSize;

    /// Allocation offset, used for validation
    UInt64 AllocationOffset;
};

HLSL_INLINE void SBTAlignInPlace(HLSL_REF(UInt64) value, uint align32) {
    if (Low(value) % align32 == 0) {
        return;
    }

    // Do it the slow way to handle carry overs
    value = AddUInt64_64(value, Make64(align32 - (Low(value) % align32), 0));
}

HLSL_INLINE SBTSharedAllocationContext SBTContextCreate() {
#ifdef __cplusplus
    return SBTSharedAllocationContext{};
#else // __cplusplus
    return (SBTSharedAllocationContext)0;
#endif // __cplusplus
}

HLSL_INLINE UInt64 GetSafeStride(UInt64 Stride) {
    if (HLSL_MAX(Low(Stride), High(Stride)) == 0) {
        return 1;
    }

    return Stride;
}

HLSL_INLINE UInt64 GetRecordInstrumentedStride(UInt64 StrideInBytes, UInt64 RecordUdStride) {
    // If there's no stride, we're still post-fixing it, but keep the no-indexing behaviour as is
    if (HLSL_MAX(Low(StrideInBytes), High(StrideInBytes)) == 0) {
        return 0;
    }

    return AddUInt64_64(StrideInBytes, RecordUdStride);
}

HLSL_INLINE UInt64 GetRecordInstrumentedSize(UInt64 SourceSizeInBytes, UInt64 SourceStrideInBytes, UInt64 PatchedStrideInBytes, UInt64 RecordUdStride)  {
    // If there's no data at all, skip this entirely
    if (HLSL_MAX(Low(SourceSizeInBytes), High(SourceSizeInBytes)) == 0) {
        return 0;
    }
    
    // If there's no stride, just postfix it
    if (HLSL_MAX(Low(SourceStrideInBytes), High(SourceStrideInBytes)) == 0) {
        return AddUInt64_64(SourceSizeInBytes, RecordUdStride);
    }

    // Retarget the stride to the patched width
    return MulUInt64_64_Low(DivUInt64_64_Low(SourceSizeInBytes, SourceStrideInBytes), PatchedStrideInBytes);
}

HLSL_INLINE UInt64 GetRecordDescriptorLength(UInt64 SizeInBytes, UInt64 StrideInBytes) {
    // If there's no data at all, skip this entirely
    if (HLSL_MAX(Low(SizeInBytes), High(SizeInBytes)) == 0) {
        return 0;
    }
    
    // If there's no stride, we only need descriptor data for the first record
    if (HLSL_MAX(Low(StrideInBytes), High(StrideInBytes)) == 0) {
        return SubUInt64_64(SizeInBytes, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    }
    
    return MulUInt64_64_Low(SubUInt64_64(StrideInBytes, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES), DivUInt64_64_Low(SizeInBytes, StrideInBytes));
}

HLSL_INLINE UInt64 SBTContextSetup(HLSL_REF(SBTSharedAllocationContext) Context, D3D12_DISPATCH_RAYS_DESC Desc) {
    // We really just need two dwords, but alignment requirements mean that we have to increment by the full alignment
    uint RecordUdStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
    
    // Patched description
    Context.Dispatch.Width = Desc.Width;
    Context.Dispatch.Height = Desc.Height;
    Context.Dispatch.Depth = Desc.Depth;

    // Set ray-gen indexing
    Context.Dispatch.RayGenerationShaderRecord.SizeInBytes = AddUInt64_64(Desc.RayGenerationShaderRecord.SizeInBytes, RecordUdStride);

    // Set callable indexing
    Context.Dispatch.CallableShaderTable.StrideInBytes = GetRecordInstrumentedStride(Desc.CallableShaderTable.StrideInBytes, RecordUdStride);
    Context.Dispatch.CallableShaderTable.SizeInBytes = GetRecordInstrumentedSize(Desc.CallableShaderTable.SizeInBytes, Desc.CallableShaderTable.StrideInBytes, Context.Dispatch.CallableShaderTable.StrideInBytes, RecordUdStride);

    // Set hit indexing
    Context.Dispatch.HitGroupTable.StrideInBytes = GetRecordInstrumentedStride(Desc.HitGroupTable.StrideInBytes, RecordUdStride);
    Context.Dispatch.HitGroupTable.SizeInBytes = GetRecordInstrumentedSize(Desc.HitGroupTable.SizeInBytes, Desc.HitGroupTable.StrideInBytes, Context.Dispatch.HitGroupTable.StrideInBytes, RecordUdStride);

    // Set miss properties
    Context.Dispatch.MissShaderTable.StrideInBytes = GetRecordInstrumentedStride(Desc.MissShaderTable.StrideInBytes, RecordUdStride);
    Context.Dispatch.MissShaderTable.SizeInBytes = GetRecordInstrumentedSize(Desc.MissShaderTable.SizeInBytes, Desc.MissShaderTable.StrideInBytes, Context.Dispatch.MissShaderTable.StrideInBytes, RecordUdStride);

    // Descriptor lengths
    Context.RayGenDescriptorLength   = GetRecordDescriptorLength(Desc.RayGenerationShaderRecord.SizeInBytes, 0);
    Context.CallableDescriptorLength = GetRecordDescriptorLength(Desc.CallableShaderTable.SizeInBytes, Desc.CallableShaderTable.StrideInBytes);
    Context.HitDescriptorLength      = GetRecordDescriptorLength(Desc.HitGroupTable.SizeInBytes, Desc.HitGroupTable.StrideInBytes);
    Context.MissDescriptorLength     = GetRecordDescriptorLength(Desc.MissShaderTable.SizeInBytes, Desc.MissShaderTable.StrideInBytes);

    // Total allocation size for patched records
    Context.AllocationSize = 0;
    SBTAlignInPlace(Context.AllocationSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    Context.AllocationSize = AddUInt64_64(Context.AllocationSize, Context.Dispatch.RayGenerationShaderRecord.SizeInBytes);
    SBTAlignInPlace(Context.AllocationSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    Context.AllocationSize = AddUInt64_64(Context.AllocationSize, Context.Dispatch.CallableShaderTable.SizeInBytes);
    SBTAlignInPlace(Context.AllocationSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    Context.AllocationSize = AddUInt64_64(Context.AllocationSize, Context.Dispatch.HitGroupTable.SizeInBytes);
    SBTAlignInPlace(Context.AllocationSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    Context.AllocationSize = AddUInt64_64(Context.AllocationSize, Context.Dispatch.MissShaderTable.SizeInBytes);
    Context.AllocationSize = AddUInt64_64(Context.AllocationSize, Context.RayGenDescriptorLength);
    Context.AllocationSize = AddUInt64_64(Context.AllocationSize, Context.CallableDescriptorLength);
    Context.AllocationSize = AddUInt64_64(Context.AllocationSize, Context.HitDescriptorLength);
    Context.AllocationSize = AddUInt64_64(Context.AllocationSize, Context.MissDescriptorLength);

    // Terminator for zero sized buffers
    Context.AllocationSize = AddUInt64_64(Context.AllocationSize, Make64(sizeof(uint), 0));

    // OK
    return Context.AllocationSize;
}

HLSL_INLINE D3D12_DISPATCH_RAYS_DESC SBTContextPatch(HLSL_REF(SBTSharedAllocationContext) Context, UInt64 BaseAddress) {
    // Current patched offset
    UInt64 patchedOffset = 0;

    // Offset into ray generation
    SBTAlignInPlace(patchedOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    Context.Dispatch.RayGenerationShaderRecord.StartAddress = AddUInt64_64(BaseAddress, patchedOffset);
    patchedOffset = AddUInt64_64(patchedOffset, Context.Dispatch.RayGenerationShaderRecord.SizeInBytes);

    // Offset into callable
    SBTAlignInPlace(patchedOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    Context.Dispatch.CallableShaderTable.StartAddress = AddUInt64_64(BaseAddress, patchedOffset);
    patchedOffset = AddUInt64_64(patchedOffset, Context.Dispatch.CallableShaderTable.SizeInBytes);

    // Offset into hits
    SBTAlignInPlace(patchedOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    Context.Dispatch.HitGroupTable.StartAddress = AddUInt64_64(BaseAddress, patchedOffset);
    patchedOffset = AddUInt64_64(patchedOffset, Context.Dispatch.HitGroupTable.SizeInBytes);

    // Offset into misses
    SBTAlignInPlace(patchedOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    Context.Dispatch.MissShaderTable.StartAddress = AddUInt64_64(BaseAddress, patchedOffset);
    patchedOffset = AddUInt64_64(patchedOffset, Context.Dispatch.MissShaderTable.SizeInBytes);

    // Offset into raygen descriptor data
    Context.RayGenDescriptorOffset = patchedOffset;
    patchedOffset = AddUInt64_64(patchedOffset, Context.RayGenDescriptorLength);
    
    // Offset into callable descriptor data
    Context.CallableDescriptorOffset = patchedOffset;
    patchedOffset = AddUInt64_64(patchedOffset, Context.CallableDescriptorLength);

    // Offset into hit descriptor data
    Context.HitDescriptorOffset = patchedOffset;
    patchedOffset = AddUInt64_64(patchedOffset, Context.HitDescriptorLength);

    // Offset into miss descriptor data
    Context.MissDescriptorOffset = patchedOffset;
    patchedOffset = AddUInt64_64(patchedOffset, Context.MissDescriptorLength);

    // Validate expected offsets
#ifdef __cplusplus
    ASSERT(patchedOffset == Context.AllocationSize - sizeof(uint32_t), "Unexpected offset");
#endif // __cplusplus

    // OK
    Context.AllocationOffset = patchedOffset;
    return Context.Dispatch;
}
