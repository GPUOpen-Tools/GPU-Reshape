// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
#include <Backends/Vulkan/Allocation/MirrorAllocation.h>

// Common
#include <Common/Containers/ReferenceObject.h>
#include <Common/ComRef.h>

// Forward declarations
struct DeviceDispatchTable;
struct VirtualResourceMapping;
class DeviceAllocator;

class PhysicalResourceMappingTablePersistentVersion : public ReferenceObject {
public:
    COMPONENT(PhysicalResourceMappingTable);

    /// Constructor
    /// \param table parent device table 
    /// \param allocator device allocator
    /// \param count number of virtual mappings
    PhysicalResourceMappingTablePersistentVersion(DeviceDispatchTable* table, const ComRef<DeviceAllocator>& allocator, uint32_t count);

    /// Destructor
    ~PhysicalResourceMappingTablePersistentVersion();

    /// Mapped virtual entries
    VirtualResourceMapping* virtualMappings{nullptr};

    /// Underlying allocation
    MirrorAllocation allocation;

    /// Buffer handles
    VkBuffer hostBuffer{nullptr};
    VkBuffer deviceBuffer{nullptr};

    /// Descriptor handles
    VkBufferView deviceView{nullptr};

private:
    /// Device table
    DeviceDispatchTable* table;

    /// Components
    ComRef<DeviceAllocator> deviceAllocator{};
};
