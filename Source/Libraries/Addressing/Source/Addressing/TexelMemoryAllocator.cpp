﻿// 
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

// Addressing
#include <Addressing/TexelMemoryAllocator.h>
#include <Addressing/TexelMemoryDWordFields.h>

// Backend
#include <Backend/Scheduler/IScheduler.h>
#include <Backend/Scheduler/SchedulerTileMapping.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/Diagnostic/DiagnosticFatal.h>

// Common
#include <Common/Registry.h>

/// Total number of texel blocks, each block has 32 texels
static constexpr uint32_t kMaxTrackedTexelBlocks = UINT32_MAX; // ~4gb

/// Max number of tracked texels
static constexpr uint64_t kMaxTrackedTexels = kMaxTrackedTexelBlocks * 32ull; // 128gb of R1

/// Debugging toggle
/// Useful for validating incorrect tile mappings
#define USE_TILED_RESOURCES 1

bool TexelMemoryAllocator::Install(size_t requestedTexels) {
    // Default size?
    if (!requestedTexels) {
        requestedTexels = kMaxTrackedTexels;
    }
    
    // Try to get data host
    shaderDataHost = registry->Get<IShaderDataHost>();
    if (!shaderDataHost) {
        return false;
    }

    // Try to get scheduler
    scheduler = registry->Get<IScheduler>();
    if (!scheduler) {
        return false;
    }

    // Determine number of (aligned) blocks
    uint64_t blockCount = (requestedTexels + 31) / 32;

    // Snap to next power of two
    blockCapacityAlignPow2 = std::max(std::bit_ceil(blockCount - 1ull), 1ull);

    // Just because we want a lot of texels, doesn't mean the hardware supports it
    // Query max, and if exceeding align to a safe (lower) power of two
    uint64_t hardwareTexelLimit = shaderDataHost->GetCapabilityTable().bufferMaxElementCount;
    if (blockCapacityAlignPow2 > hardwareTexelLimit) {
        blockCapacityAlignPow2 = std::min(blockCapacityAlignPow2, std::bit_floor(hardwareTexelLimit));
    }

    // We are always allocating pow2 - 1 to stay within numeric limits
    blockCapacityAlignPow2--;
    
    // Total number of texels
    texelCapacity = blockCapacityAlignPow2 * 32;
    
    // Create residency allocator
    tileResidencyAllocator.Install(blockCapacityAlignPow2 * sizeof(uint32_t));

    // Create buddy allocator (+1 for pow2 alignment)
    texelBuddyAllocator.Install(blockCapacityAlignPow2 + 1u);
        
    // Allocate texel mask buffer
    texelBlocksBufferID = shaderDataHost->CreateBuffer(ShaderDataBufferInfo {
#if USE_TILED_RESOURCES
        .elementCount = blockCapacityAlignPow2,
        .format = Backend::IL::Format::R32UInt,
        .flagSet = ShaderDataBufferFlag::Tiled
#else // USE_TILED_RESOURCES 
        .elementCount = 512'000'000,
        .format = Backend::IL::Format::R32UInt
#endif // USE_TILED_RESOURCES
    });

    // OK
    return true;
}

static uint32_t Cast32Checked(uint64_t value) {
    ASSERT(value < std::numeric_limits<uint32_t>::max(), "Texel indexing out of bounds");
    return static_cast<uint32_t>(value);
}

TexelMemoryAllocation TexelMemoryAllocator::Allocate(const ResourceInfo &info) {
    TexelMemoryAllocation out;

    // Get the addressing info
    out.addressInfo = addressAllocator.GetAllocationInfo(info, false);

    // Determine the number of texel blocks needed
    out.texelBlockCount = Cast32Checked((out.addressInfo.texelCount + 31) / 32);

    // Number of header dwords
    out.headerDWordCount = static_cast<uint32_t>(TexelMemoryDWordFields::Count) + static_cast<uint32_t>(out.addressInfo.subresourceOffsets.Size());

    // Create underlying allocation
    // +1 for safety padding on region writes
    uint32_t allocationDWords = out.headerDWordCount + out.texelBlockCount + 1u;
    out.buddy = texelBuddyAllocator.Allocate(allocationDWords);

    // Report buddy exhaustion
    if (out.buddy.offset == kInvalidBuddyAllocation.offset) {
        ReportFatalExhaustion();
    }

    // Just assume the starting offset from the buddy allocation
    out.texelBaseBlock = Cast32Checked(out.buddy.offset);
    if (out.texelBaseBlock + allocationDWords >= blockCapacityAlignPow2) {
        ReportFatalExhaustion();
    }
        
    // Allocate all tiles in range
#if USE_TILED_RESOURCES
    tileResidencyAllocator.Allocate(
        out.texelBaseBlock * sizeof(uint32_t),
        allocationDWords * sizeof(uint32_t)
    );
#endif // USE_TILED_RESOURCES

    // OK
    return out;
}

void TexelMemoryAllocator::ReportFatalExhaustion() {
    Backend::DiagnosticFatal(
        "Texel Memory Exhaustion",
        "GPU Reshape has exhausted the internal texel memory address range of {} blocks ({} unique texels or bytes). "
        "Texel addressing is limited by hardware texel addressing constraints, will be improved in the future.\n\n"
        "To work around this issue, disable Texel Addressing in the Launch configuration, or in Settings.",
        blockCapacityAlignPow2,
        blockCapacityAlignPow2 * 32u
    );
}

void TexelMemoryAllocator::Initialize(CommandBuilder &builder, const TexelMemoryAllocation &allocation, uint32_t failureBlockCode) const {
    TrivialStackVector<uint32_t, 64> headerDWords;
    headerDWords.Resize(allocation.headerDWordCount);

    // DW0, number of subresources
    headerDWords[static_cast<uint32_t>(TexelMemoryDWordFields::SubresourceCount)] = static_cast<uint32_t>(allocation.addressInfo.subresourceOffsets.Size());

    // DW1, special failure block
    headerDWords[static_cast<uint32_t>(TexelMemoryDWordFields::FailureBlock)] = failureBlockCode;

    // DW2, number of blocks
    ASSERT(allocation.texelBlockCount > 0, "Invalid allocation");
    headerDWords[static_cast<uint32_t>(TexelMemoryDWordFields::TexelCount)] = allocation.texelBlockCount * 32u;

    // DW3 .. n, all subresource offsets
    for (size_t i = 0; i < allocation.addressInfo.subresourceOffsets.Size(); i++) {
        headerDWords[static_cast<uint32_t>(TexelMemoryDWordFields::SubresourceOffsetStart) + i] = static_cast<uint32_t>(allocation.addressInfo.subresourceOffsets[i]);
    }

    // Fill resource header
    builder.StageBuffer(texelBlocksBufferID, allocation.texelBaseBlock * sizeof(uint32_t), sizeof(uint32_t) * headerDWords.Size(), headerDWords.Data());

    // Clear all states
    builder.ClearBuffer(
        texelBlocksBufferID,
        (allocation.texelBaseBlock + allocation.headerDWordCount) * sizeof(uint32_t),
        (allocation.texelBlockCount) * sizeof(uint32_t),
        0u
    );

    // Cleanup
    headerDWords.Clear();
}

void TexelMemoryAllocator::StageFailureCode(CommandBuilder &builder, const TexelMemoryAllocation &allocation, uint32_t failureBlockCode) const {
    builder.StageBuffer(
        texelBlocksBufferID,
        (allocation.texelBaseBlock + static_cast<uint32_t>(TexelMemoryDWordFields::FailureBlock)) * sizeof(uint32_t),
        sizeof(uint32_t),
        &failureBlockCode
    );
}

void TexelMemoryAllocator::UpdateResidency(Queue queue) {
    // All mappings
    std::vector<SchedulerTileMapping> tileMappings;
    tileMappings.reserve(tileResidencyAllocator.GetRequestCount());

    // Map all new requests
    for (uint32_t i = 0; i < tileResidencyAllocator.GetRequestCount(); i++) {
        const TileMappingRequest& request = tileResidencyAllocator.GetRequest(i);

        // Create mapping and push for mapping
        tileMappings.push_back(SchedulerTileMapping {
            .mapping = shaderDataHost->CreateMapping(texelBlocksBufferID, request.tileCount),
            .tileOffset = request.tileOffset,
            .tileCount = request.tileCount
        });
    }

    // Cleanup
    tileResidencyAllocator.ClearRequests();

    // Create the tile mappings for the new resource
    scheduler->MapTiles(queue, texelBlocksBufferID, static_cast<uint32_t>(tileMappings.size()), tileMappings.data());
}

void TexelMemoryAllocator::Free(const TexelMemoryAllocation &allocation) {
    // Free the range from the buddy allocator,
    // tiles are kept resident.
    texelBuddyAllocator.Free(allocation.buddy);
}
