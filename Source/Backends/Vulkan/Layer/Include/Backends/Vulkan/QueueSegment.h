// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
#include <Backends/Vulkan/Vulkan.h>

// Common
#include <Common/Assert.h>
#include <Common/Containers/ReferenceObject.h>

// Std
#include <vector>

// Forward declarations
struct DeviceDispatchTable;

struct QueueSegment : public ReferenceObject {
    /// Add a referenced object to the GPU lifetime of this queue allocation
    ///   ? Note, not immediate, lifetime completion is checked when queried
    /// \param obj the referenced object, once the gpu has exhausted the allocation, the objects are released
    void AddLifetime(ReferenceObject* obj) {
        obj->AddUser();
        gpuReferences.emplace_back(obj);
    }

    /// Clear this allocation
    void Clear() {
        ASSERT(gpuReferences.empty(), "Dangling gpu references in QueueSegment");
        fence = nullptr;
        finished = false;
    }

    /// Query this allocation for completion
    /// \return true if the allocation has completed
    bool Query();

    /// Query this allocation for completion, do not invoke releasing for gpu lifetime references
    /// \return true if the allocation has completed
    bool QueryNoRelease();

    /// Parent dispatch table
    DeviceDispatchTable* table;

    /// Segment GPU -> CPU fence
    VkFence fence;

    /// GPU lifetime references
    std::vector<ReferenceObject*> gpuReferences;

private:
    /// Cached fence state
    bool finished{false};
};