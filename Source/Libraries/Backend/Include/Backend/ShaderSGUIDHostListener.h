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

#pragma once

// Backend
#include "IShaderSGUIDHost.h"

// Bridge
#include <Bridge/IBridgeListener.h>

// Common
#include <Common/IComponent.h>
#include <Common/ComRef.h>

// Std
#include <unordered_map>
#include <vector>

// Forward declarations
class IShaderSGUIDHost;

class ShaderSGUIDHostListener : public TComponent<ShaderSGUIDHostListener>, public IBridgeListener {
public:
    COMPONENT(ShaderSGUIDHostListener);

    /// Get the mapping for a given sguid
    /// \param sguid
    /// \return
    ShaderSourceMapping GetMapping(ShaderSGUID sguid);

    /// Get the source code for a binding
    /// \param sguid the shader sguid
    /// \return the source code, empty if not found
    std::string_view GetSource(ShaderSGUID sguid);

    /// Overrides
    void Handle(const MessageStream *streams, uint32_t count) override;

private:
    struct Entry {
        ShaderSourceMapping mapping;
        std::string         contents;
    };

    /// Lookup
    std::vector<Entry> sguidLookup;
};
