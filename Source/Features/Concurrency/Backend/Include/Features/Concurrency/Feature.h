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
#include <Backend/ShaderData/IShaderDataHost.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/VisitContext.h>

// Message
#include <Message/MessageStream.h>

// Std
#include <atomic>

// Forward declarations
class IShaderSGUIDHost;

class ConcurrencyFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(ConcurrencyFeature);

    /// IFeature
    bool Install() override;
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

public:
    /// Proxies
    void OnDrawInstanced(CommandContext* context, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void OnDrawIndexedInstanced(CommandContext* context, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
    void OnDispatch(CommandContext* context, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ);
    void OnDispatchMesh(CommandContext *context, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ);

private:
    /// Hosts
    ComRef<IShaderSGUIDHost> sguidHost{nullptr};
    ComRef<IShaderDataHost> shaderDataHost{nullptr};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shader data
    ShaderDataID lockBufferID{InvalidShaderDataID};
    ShaderDataID eventID{InvalidShaderDataID};

    /// Cyclic event counter
    std::atomic<uint32_t> eventCounter{720};

    /// Shared stream
    MessageStream stream;
};