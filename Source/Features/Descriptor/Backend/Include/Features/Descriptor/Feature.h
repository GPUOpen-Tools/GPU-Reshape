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

// Common
#include <Common/ComRef.h>

// Backend
#include <Backend/IFeature.h>
#include <Backend/IShaderFeature.h>
#include <Backend/ShaderExport.h>
#include <Backend/IL/BasicBlock.h>
#include <Backend/IL/VisitContext.h>
#include <Backend/IL/ResourceTokenType.h>

// Message
#include <Message/MessageStream.h>

// Std
#include <atomic>

// Forward declarations
class IShaderSGUIDHost;

class DescriptorFeature final : public IFeature, public IShaderFeature {
public:
    COMPONENT(DescriptorFeature);

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

private:
    /// Inject instrumentation for a given resource
    /// \param program source program
    /// \param function source function
    /// \param it source instruction reference from which instrumentation occurs, potentially safe-guarded
    /// \param resource resource to validate
    /// \param compileTypeLiteral expected compile type value
    /// \param config instrumentation configuration
    /// \return moved iterator
    IL::BasicBlock::Iterator InjectForResource(IL::Program& program, IL::Function& function, IL::BasicBlock::Iterator it, IL::ID resource, Backend::IL::ResourceTokenType compileTypeLiteral, const struct SetInstrumentationConfigMessage& config);

private:
    /// Hosts
    ComRef<IShaderSGUIDHost> sguidHost{nullptr};

    /// Export id for this feature
    ShaderExportID exportID{};

    /// Shared stream
    MessageStream stream;
};