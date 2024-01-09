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
#include <Backends/DX12/Controllers/IController.h>

// Message
#include <Message/MessageStream.h>

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/ComRef.h>

// Std
#include <vector>
#include <mutex>

// Forward declarations
class Registry;
class Dispatcher;
class IBridge;
class ShaderCompiler;
struct DeviceState;
struct ReferenceObject;

class MetadataController final : public IController, public IBridgeListener {
public:
    COMPONENT(MetadataController);

    MetadataController(DeviceState* device);

    /// Install the controller
    bool Install();

    /// Uninstall the controller
    void Uninstall();

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) final;

    /// Commit all changes
    void Commit();

protected:
    /// Message handlers
    void OnMessage(const struct GetPipelineNameMessage& message);
    void OnMessage(const struct GetShaderCodeMessage& message);
    void OnMessage(const struct GetShaderILMessage& message);
    void OnMessage(const struct GetShaderBlockGraphMessage& message);
    void OnMessage(const struct GetObjectStatesMessage& message);
    void OnMessage(const struct GetShaderUIDRangeMessage& message);
    void OnMessage(const struct GetPipelineUIDRangeMessage& message);
    void OnMessage(const struct GetShaderSourceMappingMessage& message);

private:
    DeviceState* device;

    /// Owning bridge, stored as naked pointer for referencing reasons
    IBridge* bridge{nullptr};

    /// Components
    ComRef<ShaderCompiler> shaderCompiler;

    /// Pending response stream
    MessageStream stream;

    /// Pending segment mapping stream
    MessageStream segmentMappingStream;

    /// Shared lock
    std::mutex mutex;
};
