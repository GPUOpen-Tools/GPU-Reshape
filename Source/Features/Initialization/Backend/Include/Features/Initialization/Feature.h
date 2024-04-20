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
#include <Common/ComRef.h>

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderExport.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/VisitContext.h>
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/ShaderProgram/ShaderProgram.h>
#include <Backend/Resource/TexelAddressAllocator.h>

// Message
#include <Message/MessageStream.h>

// Std
#include <mutex>
#include <unordered_map>

// Forward declarations
class IShaderSGUIDHost;
class MaskBlitShaderProgram;
class MaskCopyRangeShaderProgram;
class IShaderProgramHost;
struct CommandBuffer;

class InitializationFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(InitializationFeature);

    /// IFeature
    bool Install() override;
    bool PostInstall() override;
    FeatureInfo GetInfo() override;
    FeatureHookTable GetHookTable() override;
    void CollectMessages(IMessageStorage *storage) override;

    /// IShaderFeature
    void CollectExports(const MessageStream &exports) override;
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
    void OnClearResource(CommandContext* context, const ResourceInfo& buffer);
    void OnWriteResource(CommandContext* context, const ResourceInfo& buffer);
    void OnBeginRenderPass(CommandContext* context, const RenderPassInfo& passInfo);
    void OnSubmitBatchBegin(const SubmitBatchHookContexts& hookContexts, const CommandContextHandle *contexts, uint32_t contextCount);
    void OnJoin(CommandContextHandle contextHandle);

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

    /// Shader data
    ShaderDataID puidMemoryBaseBufferID{InvalidShaderDataID};
    ShaderDataID texelMaskBufferID{InvalidShaderDataID};

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
        /// Absolute memory offset
        uint64_t base{0};

        /// Relative offset, aligned to 32
        uint32_t baseAlign32{0};

        /// Length of this allocation
        uint64_t length{0};
    };

    /// Shared texel allocator
    TexelAddressAllocator addressAllocator;

    /// All allocations
    std::unordered_map<uint64_t, Allocation> allocations;

private:
    struct CommandContextInfo {
        /// The next committed bases upon join
        uint64_t committedInitializationHead = 0ull;
        uint64_t committedMappingHead = 0ull;
    };

    struct InitialiationTag {
        ResourceInfo info;
        uint32_t srb{0};
    };

    struct MappingTag {
        uint64_t puid{0};
        uint32_t memoryBaseAlign32{0};
    };

    /// Context lookup
    std::map<CommandContextHandle, CommandContextInfo> commandContexts;

    /// Current queues, base indicated by commit
    std::vector<InitialiationTag> pendingInitializationQueue;
    std::vector<MappingTag> pendingMappingQueue;

    /// The current committed bases
    /// All pending initializations use this value as the base commit id
    uint64_t committedInitializationBase = 0ull;
    uint64_t committedMappingBase = 0ull;
    
private:
    /// Shared lock
    std::mutex mutex;

    /// Current initialization mask
    std::unordered_map<uint64_t, uint32_t> puidSRBInitializationMap;
};
