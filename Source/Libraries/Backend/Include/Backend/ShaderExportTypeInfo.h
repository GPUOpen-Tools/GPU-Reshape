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

// Message
#include <Message/Message.h>

// Std
#include <cstdint>

/// Shader export type metadata
struct ShaderExportTypeInfo {
    template<typename T>
    static ShaderExportTypeInfo FromType() {
        using ShaderExport = typename T::ShaderExport;
        using Schema = typename T::Schema;

        ShaderExportTypeInfo info{};
        info.messageSchema = Schema::GetSchema(T::kID);
        info.noSGUID = ShaderExport::kNoSGUID;
        info.structured = ShaderExport::kStructured;
        info.typeSize = sizeof(T);
        return info;
    }

    MessageSchema messageSchema{};
    bool noSGUID{false};
    bool structured{false};
    size_t typeSize{0};
};
