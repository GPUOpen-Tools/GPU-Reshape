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
#include <Backends/DX12/Detour.Gen.h>
#include <Backends/DX12/TrackedObject.h>
#include <Backends/DX12/DependentObject.h>
#include <Backends/DX12/Compiler/ShaderSet.h>
#include <Backends/DX12/Resource/HeapTable.h>
#include <Backends/DX12/Resource/ResourceVirtualAddressTable.h>
#include <Backends/DX12/Resource/PhysicalResourceIdentifierMap.h>
#include <Backends/DX12/FeatureProxies.Gen.h>
#include <Backends/DX12/ShaderData/ConstantShaderDataBuffer.h>

// Backend
#include <Backend/Environment.h>
#include <Backend/EventDataStack.h>

// Bridge
#include <Bridge/Log/LogBuffer.h>

// Common
#include <Common/ComRef.h>
#include <Common/Registry.h>
#include <Common/Allocators.h>
#include <Common/IntervalAction.h>

// Forward declarations
class ShaderSet;
class IFeature;
struct ShaderState;
struct CommandQueueState;
struct ResourceState;
struct PipelineState;
class InstrumentationController;
class FeatureController;
class MetadataController;
class VersioningController;
class PDBController;
class IBridge;
class ShaderExportHost;
class ShaderDataHost;
class ShaderExportStreamer;
class ShaderSGUIDHost;
class DeviceAllocator;
class ShaderProgramHost;
class Scheduler;

struct __declspec(uuid("548FDFD6-37E2-461C-A599-11DA5290F06E")) DeviceState {
    DeviceState(const Allocators& allocators)
        : allocators(allocators),
          states_Shaders(allocators.Tag(kAllocTracking)),
          states_Pipelines(allocators.Tag(kAllocTracking)),
          states_Queues(allocators.Tag(kAllocTracking)),
          states_Resources(allocators.Tag(kAllocTracking)),
          cpuHeapTable(allocators.Tag(kAllocTracking)),
          gpuHeapTable(allocators.Tag(kAllocTracking)),
          virtualAddressTable(allocators.Tag(kAllocTracking)),
          physicalResourceIdentifierMap(allocators.Tag(kAllocPRMT)),
          dependencies_shaderPipelines(allocators.Tag(kAllocTracking)),
          features(allocators),
          featureHookTables(allocators) { }
    
    ~DeviceState();

    /// Owned object
    ID3D12Device* object{nullptr};

    /// Unique identifier
    uint32_t uid{UINT32_MAX};
    
    /// Shared allocators
    Allocators allocators;

    /// Shared registry
    Registry registry;

    /// All shared shader sets
    ShaderSet shaderSet;

    /// Message bridge
    ComRef<IBridge> bridge;

    /// Shared hosts
    ComRef<ShaderExportHost> exportHost;
    ComRef<ShaderDataHost> shaderDataHost;

    /// Shared export streamer
    ComRef<ShaderExportStreamer> exportStreamer;

    /// Shared scheduler
    ComRef<Scheduler> scheduler;

    /// Shared device allocator
    ComRef<DeviceAllocator> deviceAllocator;

    /// Shared SGUID host
    ComRef<ShaderSGUIDHost> sguidHost{nullptr};

    /// Tracked objects
    TrackedObject<ShaderState> states_Shaders;
    TrackedObject<PipelineState> states_Pipelines;
    TrackedObject<CommandQueueState> states_Queues;
    TrackedObject<ResourceState> states_Resources;

    /// Sorted heap tables
    HeapTable cpuHeapTable;
    HeapTable gpuHeapTable;

    /// Sorted virtual address table
    ResourceVirtualAddressTable virtualAddressTable;

    /// Physical identifier map
    PhysicalResourceIdentifierMap physicalResourceIdentifierMap;

    /// Dependency objects
    DependentObject<ShaderState, PipelineState> dependencies_shaderPipelines;

    /// Controllers
    ComRef<InstrumentationController> instrumentationController{nullptr};
    ComRef<FeatureController> featureController{nullptr};
    ComRef<MetadataController> metadataController{nullptr};
    ComRef<VersioningController> versioningController{nullptr};
    ComRef<PDBController> pdbController{nullptr};

    /// User programs
    ComRef<ShaderProgramHost> shaderProgramHost{nullptr};

    /// Shared remapping table
    EventDataStack::RemappingTable eventRemappingTable;
    ShaderConstantsRemappingTable  constantRemappingTable;

    /// Pre-populated proxies
    ID3D12GraphicsCommandListFeatureProxies commandListProxies;

    /// Shared logging buffer
    LogBuffer logBuffer;

    /// Optional environment, ignored if creation parameters supply a registry
    Backend::Environment environment;

    /// Environment actions
    IntervalAction environmentUpdateAction = IntervalAction::FromMS(1000);

    /// All features
    Vector<ComRef<IFeature>> features;
    Vector<FeatureHookTable> featureHookTables;
};
