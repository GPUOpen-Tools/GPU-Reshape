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
#include <Backends/DX12/Resource/VirtualResourceMapping.h>
#include <Backends/DX12/Export/ShaderExportStreamState.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>

/// Forward declarations
struct DeviceState;
class DeviceAllocator;
struct ResourceState;
struct VirtualAddressMappingTablePersistentVersion;

class VirtualAddressMappingTable : public TComponent<DeviceAllocator> {
public:
    COMPONENT(VirtualAddressMappingTable);
    
    /// Constructor
    VirtualAddressMappingTable(DeviceState* device, const ComRef<DeviceAllocator>& deviceAllocator);
    ~VirtualAddressMappingTable();
    
    /// Allocate a new peristent version
    VirtualAddressMappingTablePersistentVersion* Allocate();
    
    /// Free a non-live version
    void Free(VirtualAddressMappingTablePersistentVersion* version);
    
    /// Commit a version with the current set of virtual addresses
    void Commit(VirtualAddressMappingTablePersistentVersion* version);
    
    /// Map a given descriptor handle to that of the version
    void MapHandle(VirtualAddressMappingTablePersistentVersion* version, D3D12_CPU_DESCRIPTOR_HANDLE handle);

private:
    /// Destruct a version
    void Destruct(VirtualAddressMappingTablePersistentVersion* version);
    
    /// Prune all irrelevant versions
    void Prune(uint64_t count);

private:
    /// All free versions
    std::vector<VirtualAddressMappingTablePersistentVersion*> freeVersions;
    
    /// Shared lock
    std::mutex mutex;

private:
    DeviceState* device{nullptr};
    ComRef<DeviceAllocator> deviceAllocator;
};
