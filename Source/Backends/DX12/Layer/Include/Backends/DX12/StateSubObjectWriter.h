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

// Layer
#include "DX12.h"

struct StateSubObjectWriter {
    /// Constructor
    /// \param desc given description
    StateSubObjectWriter(const Allocators& allocators) : entries(allocators), buffer(allocators) {
        
    }

    /// Get the size of a type
    /// \param type given type
    /// \return byte size, 0 if invalid
    static uint64_t GetSize(D3D12_STATE_SUBOBJECT_TYPE type) {
        switch (type) {
            default:
                ASSERT(false, "Invalid sub-object type");
                return 0;
            case D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG:
                return sizeof(D3D12_STATE_OBJECT_CONFIG);
            case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
            case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
                return sizeof(ID3D12RootSignature*);
            case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
                return sizeof(D3D12_NODE_MASK);
            case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
                return sizeof(D3D12_DXIL_LIBRARY_DESC);
            case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
                return sizeof(D3D12_EXISTING_COLLECTION_DESC);
            case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
                return sizeof(D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION);
            case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
                return sizeof(D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION);
            case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
                return sizeof(D3D12_RAYTRACING_SHADER_CONFIG);
            case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
                return sizeof(D3D12_RAYTRACING_PIPELINE_CONFIG);
            case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
                return sizeof(D3D12_HIT_GROUP_DESC);
            case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1:
                return sizeof(D3D12_RAYTRACING_PIPELINE_CONFIG1);
        }
    }

    /// Read a data parameter, helper function
    /// \param data subobject data
    /// \return data copy
    template<typename T>
    static T Read(const D3D12_STATE_SUBOBJECT& data) {
        return *static_cast<const T*>(data.pDesc);
    }

    /// Add a new sub object
    /// \param type given type
    /// \return data, length must be GetSize(type)
    void Add(D3D12_STATE_SUBOBJECT_TYPE type, const void* data) {
        // Add entry
        entries.push_back(SubObject {
            .type = type,
            .offset = buffer.size()
        });

        auto ptr = static_cast<const uint8_t*>(data);
        buffer.insert(buffer.end(), ptr, ptr + GetSize(type));
    }

    /// Add a new sub object
    /// \param type given type
    /// \return ptr, inline pointer data to be copied
    void AddPtr(D3D12_STATE_SUBOBJECT_TYPE type, const void* ptr) {
        ASSERT(GetSize(type) == sizeof(ptr), "Unexpected type");
        Add(type, &ptr);
    }

    /// Compile the description
    /// \param type state object type
    /// \return final description
    D3D12_STATE_OBJECT_DESC Compile(D3D12_STATE_OBJECT_TYPE type) {
        // Set data pointers
        for (SubObject& obj : entries) {
            obj.data = &buffer[obj.offset];
        }

        // Create description
        D3D12_STATE_OBJECT_DESC desc;
        desc.Type = type;
        desc.NumSubobjects = static_cast<uint32_t>(entries.size());
        desc.pSubobjects = reinterpret_cast<D3D12_STATE_SUBOBJECT*>(entries.data());
        return desc;
    }

private:
    struct SubObject {
        /// Underlying type
        D3D12_STATE_SUBOBJECT_TYPE type;

        /// Data, first an offset, then the final pointer
        union {
            const void* data;
            uint64_t offset;
        };
    };

    /// Validate against d3d12
    static_assert(sizeof(SubObject) == sizeof(D3D12_STATE_SUBOBJECT), "Unexpected size");

    /// All pending entries
    Vector<SubObject> entries;

    /// Intermediate data buffer
    Vector<uint8_t> buffer;
};
