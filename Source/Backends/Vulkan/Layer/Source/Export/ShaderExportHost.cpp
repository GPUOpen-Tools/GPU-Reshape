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

#include <Backends/Vulkan/Export/ShaderExportHost.h>

ShaderExportID ShaderExportHost::Allocate(const ShaderExportTypeInfo &typeInfo) {
    ShaderExportInfo& info = exports.emplace_back();
    info.typeInfo = typeInfo;

    // OK
    return static_cast<ShaderExportID>(exports.size() - 1);
}

void ShaderExportHost::Enumerate(uint32_t *count, ShaderExportID *out) {
    if (out) {
        for (uint32_t i = 0; i < *count; i++) {
            out[i] = i;
        }
    } else {
        *count = static_cast<uint32_t>(exports.size());
    }
}

uint32_t ShaderExportHost::GetBound() {
    return static_cast<uint32_t>(exports.size());
}

ShaderExportTypeInfo ShaderExportHost::GetTypeInfo(ShaderExportID id) {
    return exports.at(id).typeInfo;
}
