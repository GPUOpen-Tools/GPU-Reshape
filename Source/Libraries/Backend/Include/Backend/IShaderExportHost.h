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

#include "ShaderExport.h"
#include "ShaderExportTypeInfo.h"

// Common
#include <Common/IComponent.h>

class IShaderExportHost : public TComponent<IShaderExportHost> {
public:
    COMPONENT(IShaderExportHost);

    /// Allocate a shader export
    /// \param typeInfo the type information for the export
    /// \return the allocation identifier
    virtual ShaderExportID Allocate(const ShaderExportTypeInfo& typeInfo) = 0;

    /// Get the type info of an export
    /// \param id id of the export
    /// \return the type info
    virtual ShaderExportTypeInfo GetTypeInfo(ShaderExportID id) = 0;

    /// Enumerate all exports
    /// \param count if [out] is nullptr, filled with the number of exports
    /// \param out optional, if valid is filled with [count] exports
    virtual void Enumerate(uint32_t* count, ShaderExportID* out) = 0;

    /// Get the shader export bound
    /// \return the current id-limit / bound
    virtual uint32_t GetBound() = 0;

    /// Allocate a shader export
    /// \tparam T the allocation type, must be of a schema shader-export type
    /// \return the allocation identifier
    template<typename T>
    ShaderExportID Allocate() {
        return Allocate(ShaderExportTypeInfo::FromType<T>());
    }
};
