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

// Addressing
#include <Addressing/TexelAddressAllocator.h>
#include <Addressing/TileResidencyAllocator.h>
#include <Addressing/TexelMemoryAllocation.h>

// Backend
#include <Backend/Command/CommandBuilder.h>
#include <Backend/ShaderData/ShaderData.h>
#include <Backend/Scheduler/Queue.h>

// Common
#include <Common/Allocator/BuddyAllocator.h>
#include <Common/ComRef.h>

// Forward declarations
class IScheduler;
class IShaderDataHost;

class TexelMemoryAllocator : public TComponent<TexelMemoryAllocator> {
public:
    COMPONENT(TexelMemoryAllocator);

    /// Install this allocator
    /// \param texelCount maximum number of texels, if zero, assumes maximum
    /// \return success state
    bool Install(size_t texelCount = 0);

    /// Allocate a new texel memory region
    /// \param info resource to allocate for
    /// \return allocation
    TexelMemoryAllocation Allocate(const ResourceInfo& info);

    /// Initialize a resource
    /// \param builder builder to stage in
    /// \param allocation allocation to initialize
    /// \param failureBlockCode the initial failure code, use 0x0 for no failure
    void Initialize(CommandBuilder& builder, const TexelMemoryAllocation& allocation, uint32_t failureBlockCode) const;

    /// Stage a failure code
    /// \param builder builder to stage in
    /// \param allocation allocation to stage
    /// \param failureBlockCode the staged failure code, use 0x0 for no failure
    void StageFailureCode(CommandBuilder& builder, const TexelMemoryAllocation& allocation, uint32_t failureBlockCode) const;
    
    /// Update the residency on a target queue
    /// \param queue target queue
    void UpdateResidency(Queue queue);

    /// Free an allocation
    /// \param allocation allocation to free
    void Free(const TexelMemoryAllocation& allocation);

public:
    /// Get the texel block buffer
    ShaderDataID GetTexelBlocksBufferID() const {
        return texelBlocksBufferID;
    }

private:
    /// Texel address allocator
    TexelAddressAllocator addressAllocator;

    /// Residency management for the tiled resource
    TileResidencyAllocator tileResidencyAllocator;

    /// Underlying texel memory range allocator
    BuddyAllocator texelBuddyAllocator;

private:
    /// Shared components
    ComRef<IShaderDataHost> shaderDataHost;
    ComRef<IScheduler> scheduler;

    /// Contains all texel blocks
    ShaderDataID texelBlocksBufferID{InvalidShaderDataID};

private:
    // Maximum number of blocks
    size_t blockCapacityAlignPow2{0};

    /// Maximum number of texels
    size_t texelCapacity{0};
};
