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

#include <Backend/ShaderSGUIDHostListener.h>
#include <Backend/IShaderSGUIDHost.h>

// Common
#include <Common/Registry.h>

// Message
#include <Message/MessageStream.h>

// Schemas
#include <Schemas/SGUID.h>

// Std
#include <algorithm>

void ShaderSGUIDHostListener::Handle(const MessageStream *streams, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView<ShaderSourceMappingMessage> view(streams[i]);

        // Determine bound
        uint32_t sguidBound{0};
        for (auto it = view.GetIterator(); it; ++it) {
            sguidBound = std::max(sguidBound, it->sguid);
        }

        // Ensure enough space
        if (sguidBound >= sguidLookup.size()) {
            sguidLookup.resize(sguidBound + 1);
        }

        // Populate entries
        for (auto it = view.GetIterator(); it; ++it) {
            Entry& entry = sguidLookup.at(it->sguid);
            entry.mapping.shaderGUID = it->shaderGUID;
            entry.mapping.fileUID = it->fileUID;
            entry.mapping.line = it->line;
            entry.mapping.column = it->column;
            entry.contents = std::string(it->contents.View());
        }
    }
}

ShaderSourceMapping ShaderSGUIDHostListener::GetMapping(ShaderSGUID sguid) {
    if (sguidLookup.size() <= sguid) {
        return {};
    }

    return sguidLookup[sguid].mapping;
}

std::string_view ShaderSGUIDHostListener::GetSource(ShaderSGUID sguid) {
    if (sguidLookup.size() <= sguid) {
        return {};
    }

    return sguidLookup[sguid].contents;
}
