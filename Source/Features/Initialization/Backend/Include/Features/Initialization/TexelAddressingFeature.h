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
#include <Features/Initialization/TexelAddressing/FailureCode.h>

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderExport.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/VisitContext.h>
#include <Backend/IL/Emitters/Emitter.h>
#include <Backend/IL/Emitters/ResourceTokenEmitter.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/ShaderProgram/ShaderProgram.h>
#include <Backend/Scheduler/SchedulerPrimitive.h>

// Addressing
#include <Addressing/TexelMemoryAllocation.h>

// Message
#include <Message/MessageStream.h>

// Common
#include <Common/Containers/SlotArray.h>
#include <Common/ComRef.h>

// Std
#include <mutex>
#include <unordered_map>
#include <unordered_set>

// Forward declarations
class IScheduler;
class IShaderSGUIDHost;
class MaskBlitShaderProgram;
class MaskCopyRangeShaderProgram;
class TexelMemoryAllocator;
class IShaderProgramHost;
struct CommandBuffer;

class TexelAddressingInitializationFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(TexelAddressingInitializationFeature);

    /// IFeature
    bool Install() override;
    bool PostInstall() override;
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

private:
    /// Hooks
    void OnCreateResource(const ResourceCreateInfo& source);
    void OnDestroyResource(const ResourceInfo& source);
    void OnMapResource(const ResourceInfo& source);
    void OnCopyResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest);
    void OnResolveResource(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest);
    void OnClearResource(CommandContext* context, const ResourceInfo& resource);
    void OnWriteResource(CommandContext* context, const ResourceInfo& resource);
    void OnDiscardResource(CommandContext* context, const ResourceInfo& resource);
    void OnBeginRenderPass(CommandContext* context, const RenderPassInfo& passInfo);
    void OnSubmitBatchBegin(SubmissionContext& submitContext, const CommandContextHandle *contexts, uint32_t contextCount);
    void OnJoin(CommandContextHandle contextHandle);

private:
    /// Mark a resource metadata as initialized
    /// \param context destination context
    /// \param info resource metadata to mark as initialized
    void OnMetadataInitializationEvent(CommandContext* context, const ResourceInfo& info);

private:
    /// Blit a resource mask
    /// \param buffer destination command buffer
    /// \param info the resource info, contains sub-regions
    void BlitResourceMask(CommandBuffer& buffer, const ResourceInfo& info);
    
    /// Copy an existing resource mask
    /// \param buffer destination command buffer
    /// \param source the source resource info, contains sub-regions
    /// \param dest the destination resource info, contains sub-regions
    void CopyResourceMaskRange(CommandBuffer& buffer, const ResourceInfo& source, const ResourceInfo& dest);
    
    /// Copy an existing resource mask with symmetric token types
    /// \param buffer destination command buffer
    /// \param source the source resource info, contains sub-regions
    /// \param dest the destination resource info, contains sub-regions
    void CopyResourceMaskRangeSymmetric(CommandBuffer& buffer, const ResourceInfo& source, const ResourceInfo& dest);
    
    /// Copy an existing resource mask with asymmetric token types
    /// \param buffer destination command buffer
    /// \param source the source resource info, contains sub-regions
    /// \param dest the destination resource info, contains sub-regions
    void CopyResourceMaskRangeAsymmetric(CommandBuffer& buffer, const ResourceInfo& source, const ResourceInfo& dest);
    
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

private:
    template<typename T>
    struct ResourceProgram {
        /// Masking program
        ComRef<T> program;

        /// Allocated program ID
        ShaderProgramID id{InvalidShaderProgramID};
    };

    /// Create a new program
    /// @param programHost target host
    /// @param out destination program
    /// @param args all creation arguments
    /// @return success state
    template<typename T, typename... A>
    bool CreateProgram(const ComRef<IShaderProgramHost>& programHost, ResourceProgram<T>& out, A&&... args);

    /// Create a mask blitting program
    /// @param programHost target host
    /// @param type token type
    /// @param isVolumetric volumetric addressing
    /// @return success state
    bool CreateBlitProgram(const ComRef<IShaderProgramHost>& programHost, Backend::IL::ResourceTokenType type, bool isVolumetric);

    /// Create a mask blitting program
    /// @param programHost target host
    /// @param from the source token type
    /// @param to the destination token type, may be asymmetric
    /// @param isVolumetric volumetric addressing
    /// @return success state
    bool CreateCopyProgram(const ComRef<IShaderProgramHost>& programHost, Backend::IL::ResourceTokenType from, Backend::IL::ResourceTokenType to, bool isVolumetric);
    
    /// Create all blitting programs
    /// @param programHost target host
    /// @return success state
    bool CreateBlitPrograms(const ComRef<IShaderProgramHost>& programHost);
    
    /// Create all copy programs
    /// @param programHost target host
    /// @return success state
    bool CreateCopyPrograms(const ComRef<IShaderProgramHost>& programHost);

    /// Map keys
    using BlitSortKey = std::tuple<Backend::IL::ResourceTokenType, bool>;
    using CopySortKey = std::tuple<Backend::IL::ResourceTokenType, Backend::IL::ResourceTokenType, bool>;

    /// Program maps
    std::map<BlitSortKey, ResourceProgram<MaskBlitShaderProgram>> blitPrograms;
    std::map<CopySortKey, ResourceProgram<MaskCopyRangeShaderProgram>> copyPrograms;

private:
    struct Allocation {
        /// Resource info
        ResourceCreateInfo createInfo;
        
        /// The underlying allocation
        TexelMemoryAllocation memory;

        /// Assigned initial failure code
        FailureCode failureCode{FailureCode::None};

        /// Has this resource been mapped, i.e. bound to any memory?
        /// By default, resources are unmapped until requested
        bool mapped{false};

        /// Slot keys
        uint64_t pendingMappingKey{};

        /// Is a whole resource blit pending?
        bool pendingWholeResourceBlit{false};
    };

    /// Map an allocation
    /// \param allocation allocation to be mapped
    /// \param immediate if true, mapped during allocation event
    void MapAllocationNoLock(Allocation& allocation, bool immediate);

    /// Schedule a whole resource blit
    /// \param allocation allocation to be blitted
    void ScheduleWholeResourceBlit(Allocation& allocation);
    
    /// Map all pending allocations
    void MapPendingAllocationsNoLock();
    
    /// Shared texel allocator
    ComRef<TexelMemoryAllocator> texelAllocator;

    /// All allocations
    std::unordered_map<uint64_t, Allocation> allocations;

    /// All allocations pending mapping
    SlotArray<Allocation*, &Allocation::pendingMappingKey> pendingMappingAllocations;
    
private:
    struct CommandContextInfo {
        /// The next committed bases upon join
        uint64_t committedInitializationHead = 0ull;
    };

    struct InitialiationTag {
        ResourceInfo info;
        uint32_t srb{0};
    };

    struct MappingTag {
        uint64_t puid{0};
        uint32_t memoryBaseAlign32{0};
    };

    struct DiscardTag {
        uint32_t puid{0};
    };

    /// Context lookup
    std::map<CommandContextHandle, CommandContextInfo> commandContexts;

    /// Current queues, base indicated by commit
    std::vector<InitialiationTag> pendingInitializationQueue;
    std::vector<MappingTag> pendingMappingQueue;
    std::vector<DiscardTag> pendingDiscardQueue;

    /// The current committed bases
    /// All pending initializations use this value as the base commit id
    uint64_t committedInitializationBase = 0ull;

    /// Is incremental mapping enabled?
    bool incrementalMapping{false};

private:
    /// Monotonically incremented primitive counters
    uint64_t exclusiveTransferPrimitiveMonotonicCounter{0};
    uint64_t exclusiveComputePrimitiveMonotonicCounter{0};

    /// Primitives used for all synchronization
    SchedulerPrimitiveID exclusiveTransferPrimitiveID{InvalidSchedulerPrimitiveID};
    SchedulerPrimitiveID exclusiveComputePrimitiveID{InvalidSchedulerPrimitiveID};

private:
    /// Any pending compute-only synchronization?
    bool pendingComputeSynchronization{false};

    /// Is this feature currently activated?
    bool activated{false};
    
private:
    /// Shared lock
    std::mutex mutex;

    /// Current initialization mask
    std::unordered_set<uint64_t> puidSRBInitializationSet;
};
