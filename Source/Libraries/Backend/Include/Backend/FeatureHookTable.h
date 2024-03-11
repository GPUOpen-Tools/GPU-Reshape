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

// Backend
#include "CommandContextHandle.h"

// Common
#include <Common/Delegate.h>

// Std
#include <cstdint>

// Forward declarations
class CommandContext;
struct BufferDescriptor;
struct TextureDescriptor;
struct ResourceInfo;
struct RenderPassInfo;

struct SubmitBatchHookContexts {
    /// Commands injected prior all command contexts
    CommandContext* preContext{nullptr};

    /// Commands injected after all command contexts
    CommandContext* postContext{nullptr};
};

// Hook types
namespace Hooks {
    /// Invocations
    using DrawInstanced = Delegate<void(CommandContext* context, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)>;
    using DrawIndexedInstanced = Delegate<void(CommandContext* context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)>;
    using Dispatch = Delegate<void(CommandContext* context, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)>;
    using DispatchMesh = Delegate<void(CommandContext* context, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)>;

    /// Resource
    using MapResource = Delegate<void(const ResourceInfo& source)>;
    using UnmapResource = Delegate<void(const ResourceInfo& source)>;
    using CopyResource = Delegate<void(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest)>;
    using ResolveResource = Delegate<void(CommandContext* context, const ResourceInfo& source, const ResourceInfo& dest)>;
    using ClearResource = Delegate<void(CommandContext* context, const ResourceInfo& resource)>;
    using WriteResource = Delegate<void(CommandContext* context, const ResourceInfo& resource)>;

    /// Render pass
    using BeginRenderPass = Delegate<void(CommandContext* context, const RenderPassInfo& passInfo)>;
    using EndRenderPass = Delegate<void(CommandContext* context)>;

    /// Submission
    using Open = Delegate<void(CommandContext *context)>;
    using Close = Delegate<void(CommandContextHandle contextHandle)>;
    using PreSubmit = Delegate<void(const SubmitBatchHookContexts& hookContexts, const CommandContextHandle *contexts, uint32_t contextCount)>;
    using PostSubmit = Delegate<void(const CommandContextHandle *contexts, uint32_t contextCount)>;
    using Join = Delegate<void(CommandContextHandle contextHandle)>;
}

/// Contains the required hooks for a given feature
class FeatureHookTable {
public:
    /// Invocations
    Hooks::DrawInstanced drawInstanced;
    Hooks::DrawIndexedInstanced drawIndexedInstanced;
    Hooks::Dispatch dispatch;
    Hooks::DispatchMesh dispatchMesh;

    /// Resource
    Hooks::MapResource mapResource;
    Hooks::UnmapResource unmapResource;
    Hooks::CopyResource copyResource;
    Hooks::ResolveResource resolveResource;
    Hooks::ClearResource clearResource;
    Hooks::WriteResource writeResource;

    /// Render pass
    Hooks::BeginRenderPass beginRenderPass;
    Hooks::EndRenderPass endRenderPass;

    /// Submission
    Hooks::Open open;
    Hooks::Close close;
    Hooks::PreSubmit preSubmit;
    Hooks::PostSubmit postSubmit;
    Hooks::Join join;
};
