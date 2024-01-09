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

// Common
#include <Common/IComponent.h>
#include <Common/Containers/ObjectPool.h>
#include <Common/Containers/TrivialObjectPool.h>
#include <Common/ComRef.h>

// Backend
#include <Backend/ShaderExportTypeInfo.h>

// Layer
#include <Backends/Vulkan/VMA.h>
#include "ShaderExportAllocationMode.h"
#include "ShaderExportStream.h"
#include "SegmentInfo.h"

// Std
#include <vector>

// Forward declarations
struct CommandBufferObject;
struct DeviceDispatchTable;
class DeviceAllocator;

class ShaderExportStreamAllocator : public TComponent<ShaderExportStreamAllocator> {
public:
    COMPONENT(ShaderExportStreamAllocator);

    ShaderExportStreamAllocator(DeviceDispatchTable* table);

    ~ShaderExportStreamAllocator();

    bool Install();

    /// Allocate a new allocation
    /// \return the allocation
    ShaderExportSegmentInfo* AllocateSegment();

    /// Free an existing allocation
    /// \param segment allocation to be free'd
    void FreeSegment(ShaderExportSegmentInfo* segment);

    /// Set the size of a shader export stream
    ///   ! Incurs potential segmentation on the next allocation
    /// \param id the shader export id
    /// \param size the byte size of the new stream
    void SetStreamSize(ShaderExportID id, uint64_t size);

private:
    /// Allocate a new stream
    /// \param id the export id
    /// \return stream info
    ShaderExportStreamInfo AllocateStreamInfo(const ShaderExportID& id);

    /// Allocate a new counter
    /// \return counter info
    ShaderExportSegmentCounterInfo AllocateCounterInfo();

private:
    struct ExportInfo {
        ShaderExportID id{0};
        ShaderExportTypeInfo typeInfo;
        uint64_t dataSize{0};
    };

    std::vector<ExportInfo> exportInfos;

private:
    ComRef<DeviceAllocator> deviceAllocator{};

    /// Pools
    ObjectPool<ShaderExportSegmentInfo> segmentPool;
    TrivialObjectPool<ShaderExportSegmentCounterInfo> counterPool;
    TrivialObjectPool<ShaderExportStreamInfo> streamPool;

    /// Initial allocation size for all streams
    uint64_t baseDataSize = 10'000;

    ShaderExportAllocationMode allocationMode{ShaderExportAllocationMode::GlobalCyclicBufferNoOverwrite};

private:
    DeviceDispatchTable* table;
};
