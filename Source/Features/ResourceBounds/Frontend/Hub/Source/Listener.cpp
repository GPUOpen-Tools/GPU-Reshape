// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

#include <Features/ResourceBounds/Listener.h>

// Message
#include <Message/IMessageHub.h>

// Backend
#include <Backend/ShaderSGUIDHostListener.h>

// Schemas
#include <Schemas/Features/ResourceBounds.h>

// Common
#include <Common/Registry.h>
#include <Common/Format.h>

bool ResourceBoundsListener::Install() {
    hub = registry->Get<IMessageHub>();
    if (!hub) {
        return false;
    }

    // Host is optional
    sguidHost = registry->Get<ShaderSGUIDHostListener>();

    // OK
    return true;
}

void ResourceBoundsListener::Handle(const MessageStream *streams, uint32_t count) {
    lookupTable.clear();

    // Create table
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView<ResourceIndexOutOfBoundsMessage> view(streams[i]);
        for (auto it = view.GetIterator(); it; ++it) {
            lookupTable[it->GetKey()]++;
        }
    }

    // Print result
    for (auto&& kv : lookupTable) {
        auto message = ResourceIndexOutOfBoundsMessage::FromKey(kv.first);

        std::string_view source = "";

        // Get source code
        if (sguidHost && message.sguid != InvalidShaderSGUID) {
            source = sguidHost->GetSource(message.sguid);
        }

        // Compose message to hug
        hub->Add("ResourceIndexOutOfBounds", Format(
            "{} {} out of bounds\n\t{}\n",
            message.isTexture ? "texture" : "buffer",
            message.isWrite ? "write" : "read",
            source
        ), kv.second);
    }
}
