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

// Feature
#include <Features/Concurrency/TexelAddressing/Container.h>

// Common
#include <Common/ComRef.h>

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderExport.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/VisitContext.h>
#include <Backend/Scheduler/SchedulerPrimitive.h>

// Message
#include <Message/MessageStream.h>

// Forward declarations
class IShaderSGUIDHost;
class TexelMemoryAllocator;
class ConcurrencyValidationListener;
class IScheduler;

class TexelAddressingConcurrencyFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(TexelAddressingConcurrencyFeature);

    /// IFeature
    bool Install() override;
    FeatureInfo GetInfo() override;
    FeatureHookTable GetHookTable() override;
    void CollectMessages(IMessageStorage *storage) override;
    void Activate(FeatureActivationStage stage) override;
    void Deactivate() override;

    /// IShaderFeature
    void CollectExports(const MessageStream &exports) override;
    void PreInject(IL::Program &program, const MessageStreamView<> &specialization) override;
    void Inject(IL::Program &program, const MessageStreamView<> &specialization) override;

    /// Interface querying
    void *QueryInterface(ComponentID id) override {
        switch (id) {
            case IComponent::kID:
                return static_cast<IComponent*>(this);
            case IFeature::kID:
                return static_cast<IFeature*>(this);
            case IShaderFeature::kID:
                return static_cast<IShaderFeature*>(this);
        }

        return nullptr;
    }

public:
    /// Proxies
    void OnCreateResource(const ResourceCreateInfo& source);
    void OnDestroyResource(const ResourceInfo& source);
    void OnSubmitBatchBegin(SubmissionContext& submitContext, const CommandContextHandle *contexts, uint32_t contextCount);

private:
    /// Map an allocation
    /// \param allocation allocation to be mapped
    void MapAllocationNoLock(ConcurrencyContainer::Allocation& allocation);

    /// Map all pending allocations
    void MapPendingAllocationsNoLock();

private:
    /// Shared container
    ConcurrencyContainer container;

    /// Shared texel allocator
    ComRef<TexelMemoryAllocator> texelAllocator;

    /// Optional, validation listener
    ComRef<ConcurrencyValidationListener> validationListener;

private:
    struct MappingTag {
        uint64_t puid{0};
        uint32_t memoryBaseAlign32{0};
    };

    /// Current queue
    std::vector<MappingTag> pendingMappingQueue;

    /// Is incremental mapping enabled?
    bool incrementalMapping{false};

private:
    /// Monotonically incremented primitive counter
    uint64_t exclusiveTransferPrimitiveMonotonicCounter{0};

    /// Primitive used for all transfer synchronization
    SchedulerPrimitiveID exclusiveTransferPrimitiveID{InvalidSchedulerPrimitiveID};

private:
    /// Is this feature currently activated?
    bool activated{false};
    
private:
    /// Hosts
    ComRef<IShaderSGUIDHost> sguidHost{nullptr};
    ComRef<IShaderDataHost> shaderDataHost{nullptr};
    ComRef<IScheduler> scheduler{nullptr};

    /// Shader data
    ShaderDataID puidMemoryBaseBufferID{InvalidShaderDataID};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shared stream
    MessageStream stream;
};