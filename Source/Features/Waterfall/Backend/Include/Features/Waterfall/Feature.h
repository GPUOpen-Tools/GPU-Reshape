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

// Common
#include <Common/ComRef.h>

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderExport.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/Emitters/Emitter.h>

// Message
#include <Message/MessageStream.h>

// Std
#include <mutex>

namespace IL {
    struct VisitContext;
}

// Forward declarations
class IShaderSGUIDHost;
struct SetInstrumentationConfigMessage;

class WaterfallFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(WaterfallFeature);

    /// IFeature
    bool Install() override;
    FeatureInfo GetInfo() override;
    FeatureHookTable GetHookTable() override;
    void CollectMessages(IMessageStorage *storage) override;

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
    struct SharedData : public TComponent<SharedData> {
        COMPONENT(SharedData);

        /// The originating blocks of an instruction id
        std::unordered_map<IL::ID, IL::ID> instructionSourceBlocks;
    };
    
    /// Inject waterfall checks to address chains
    IL::BasicBlock::Iterator InjectAddressChain(IL::Program& program, const ComRef<SharedData>& data, const SetInstrumentationConfigMessage& config, IL::VisitContext& context, IL::BasicBlock::Iterator it);

    /// Inject the divergence checks
    IL::ID InjectRuntimeDivergenceVisitor(IL::Program& program, IL::Emitter<>& pre, const IL::AddressChainInstruction* splitInstr);

    /// Inject waterfall checks to composite extraction
    IL::BasicBlock::Iterator InjectExtract(IL::Program& program, const ComRef<SharedData>& data, IL::VisitContext& context, IL::BasicBlock::Iterator it);

private:
    /// Shader SGUID
    ComRef<IShaderSGUIDHost> sguidHost{nullptr};

    /// Export id for this feature
    ShaderExportID divergentResourceExportID{};

    /// Shared stream
    MessageStream stream;

    /// Shared lock
    std::mutex mutex;
};