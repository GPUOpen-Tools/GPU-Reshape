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

#include "Shared/ShaderRecordPatching.h"
#include "Shared/RaytracingIndirectSetup.h"
#include "Shared/ShaderBackendMessage.h"

/// All constant data
ConstantBuffer<SBTIndirectSetupConstantData> Constants : register(b0);

/// The dispatch data to patch
RWStructuredBuffer<D3D12_DISPATCH_RAYS_DESC> RWDispatchBuffer : register(u1);

/// Backend messages for diagnostics
RWStructuredBuffer<uint> RWBackendMessageBuffer : register(u2);

/// The SBT patching signature dwords
RWStructuredBuffer<uint> RWSBTPatchDispatchSignatures : register(u3);

/// The SBT patching constant datas
RWStructuredBuffer<SBTPatchConstantData> RWSBTPatchConstantData : register(u4);

/// Command count buffer
StructuredBuffer<uint> CountBuffer : register(t5);

UInt64 WriteSBTConstants(
    uint RangeIndex,
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE Source,
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE Patch,
    UInt64 DescriptorLength,
    UInt64 DescriptorVAddr) {
    // Total number of records
    UInt64 StrideOrSize = all(Source.StrideInBytes == 0) ? Source.SizeInBytes : Source.StrideInBytes;
    uint   RecordCount  = Low(DivUInt64_64_Low(Source.SizeInBytes, StrideOrSize));

    // Expected descriptor stride
    uint DescriptorStride = Low(DescriptorLength) / RecordCount;

    // Copy over the common and fill in the rest
    SBTPatchConstantData PatchData = Constants.CommonPatchConstants;
    PatchData.SourceDWordStride = Low(DivUInt64_64_Low(Source.StrideInBytes, sizeof(uint)));
    PatchData.PatchedDWordStride = Low(DivUInt64_64_Low(Patch.StrideInBytes, sizeof(uint)));
    PatchData.DescriptorConstantStart = DescriptorVAddr;
    PatchData.DescriptorConstantStride = DescriptorStride / sizeof(uint);
    PatchData.SBTRecordCount = RecordCount;
    RWSBTPatchConstantData[RangeIndex] = PatchData;

    // Report the written constant address
    return AddUInt64_64(Constants.ConstantBaseAddress, uint2(Constants.ConstantStride * RangeIndex, 0));
}

void WriteVAddr(inout uint CommandOffset, UInt64 VAddr) {
    RWSBTPatchDispatchSignatures[CommandOffset + 0] = Low(VAddr);
    RWSBTPatchDispatchSignatures[CommandOffset + 1] = High(VAddr);
    CommandOffset += 2;
}

void WriteSBTRange(
    uint RangeIndex,
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE Source,
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE Patch,
    UInt64 DescriptorLength, UInt64 DescriptorOffset) {
    // Destination command dword
    uint CommandOffset = RaytracingIndirectSetupCommandDWordStride * RangeIndex;

    // Determine the number of thread groups for SBT patching
    UInt64 StrideOrSize = all(Source.StrideInBytes == 0) ? Source.SizeInBytes : Source.StrideInBytes;
    uint   RecordCount  = Low(DivUInt64_64_Low(Source.SizeInBytes, StrideOrSize));
    uint   ThreadGroups = (RecordCount + 31) / 32;

    // Start of the descriptor data
    UInt64 DescriptorVAddr = AddUInt64_64(Constants.ScratchBaseAddress, DescriptorOffset);

    // Write constants
    UInt64 ConstantOffset = WriteSBTConstants(RangeIndex, Source, Patch, DescriptorLength, DescriptorVAddr);

    // VAddr
    WriteVAddr(CommandOffset, ConstantOffset);
    WriteVAddr(CommandOffset, Source.StartAddress);
    WriteVAddr(CommandOffset, Patch.StartAddress);
    WriteVAddr(CommandOffset, DescriptorVAddr);

    // D3D12_DISPATCH_ARGUMENTS
    RWSBTPatchDispatchSignatures[CommandOffset + 0] = ThreadGroups;
    RWSBTPatchDispatchSignatures[CommandOffset + 1] = 1;
    RWSBTPatchDispatchSignatures[CommandOffset + 2] = 1;
}

void WriteEmptySBTRange(uint RangeIndex) {
    uint CommandOffset = RaytracingIndirectSetupCommandDWordStride * RangeIndex;

    // Write dummy addresses
    WriteVAddr(CommandOffset, Constants.ConstantBaseAddress);
    WriteVAddr(CommandOffset, Constants.ConstantBaseAddress);
    WriteVAddr(CommandOffset, Constants.ConstantBaseAddress);
    WriteVAddr(CommandOffset, Constants.ConstantBaseAddress);

    // D3D12_DISPATCH_ARGUMENTS
    RWSBTPatchDispatchSignatures[CommandOffset + 0] = 0;
    RWSBTPatchDispatchSignatures[CommandOffset + 1] = 0;
    RWSBTPatchDispatchSignatures[CommandOffset + 2] = 0;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE SingleRecordStride(D3D12_GPU_VIRTUAL_ADDRESS_RANGE Range) {
    D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE Out;
    Out.StartAddress = Range.StartAddress;
    Out.SizeInBytes = Range.SizeInBytes;
    Out.StrideInBytes = Range.SizeInBytes;
    return Out;
}

[numthreads(1, 1, 1)]
void main() {
    // Out of bounds?
    if (Constants.CommandIndex >= CountBuffer[0]) {
        // Write all empty ranges
        for (uint i = 0; i < RaytracingIndirectSetupCommandRangeCount; i++) {
            WriteEmptySBTRange(i);
        }
        
        return;
    }
    
    // User description
    D3D12_DISPATCH_RAYS_DESC Desc = RWDispatchBuffer[0];

    // Create context
    SBTSharedAllocationContext Ctx = SBTContextCreate();

    // Determine the number of bytes needed
    UInt64 RequestedBytes = SBTContextSetup(Ctx, Desc);

    // If out of space, panic!
    if (Low(RequestedBytes) > Constants.ScratchByteCount) {
        SendScratchOverflowMessage(RWBackendMessageBuffer, Low(RequestedBytes));

        // Dispatch will run, make sure it doesn't do any meaningful work
        Desc.Width = 0;
        Desc.Height = 0;
        Desc.Depth = 0;

        // Write empty desc
        RWDispatchBuffer[0] = Desc;
        return;
    }

    // Write patched dispatch, always at the base address
    RWDispatchBuffer[0] = SBTContextPatch(Ctx, Constants.ScratchBaseAddress);

    // Validation
    if (Low(Ctx.AllocationOffset) != Low(Ctx.AllocationSize) - sizeof(uint)) {
        DWordArray<1> Data = { __LINE__ };
        SendAssertionMessage(RWBackendMessageBuffer, Data);
    }
    
    // Write ray-gen
    WriteSBTRange(
        0, SingleRecordStride(Desc.RayGenerationShaderRecord), SingleRecordStride(Ctx.Dispatch.RayGenerationShaderRecord),
        Ctx.RayGenDescriptorLength, Ctx.RayGenDescriptorOffset
    );

    // Write callable
    WriteSBTRange(
        1, Desc.CallableShaderTable, Ctx.Dispatch.CallableShaderTable,
        Ctx.CallableDescriptorLength, Ctx.CallableDescriptorOffset
    );

    // Write hit
    WriteSBTRange(
        2, Desc.HitGroupTable, Ctx.Dispatch.HitGroupTable,
        Ctx.HitDescriptorLength, Ctx.HitDescriptorOffset
    );

    // Write miss
    WriteSBTRange(
        3, Desc.MissShaderTable, Ctx.Dispatch.MissShaderTable,
        Ctx.MissDescriptorLength, Ctx.MissDescriptorOffset
    );
}
