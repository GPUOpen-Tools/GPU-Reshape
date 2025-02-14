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
#include "DX12.h"

// Common
#include <Common/Containers/LinearBlockAllocator.h>

struct StateSubObjectWriter {
    /// Constructor
    /// \param desc given description
    StateSubObjectWriter(const Allocators& allocators) : subObjects(allocators), allocator(allocators), pendingAssociations(allocators) {
        
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
                return sizeof(D3D12_GLOBAL_ROOT_SIGNATURE);
            case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
                return sizeof(D3D12_LOCAL_ROOT_SIGNATURE);
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

    /// Reserve a set of sub objects
    /// \param count number of objects
    void Reserve(uint32_t count) {
        subObjects.reserve(count);
    }

    /// Get the future address of a sub-object
    /// \param index index, must be reserved
    /// \return address
    const D3D12_STATE_SUBOBJECT* FutureAddressOf(uint32_t index) {
        ASSERT(subObjects.capacity() > index, "Out of bounds address");
        return reinterpret_cast<const D3D12_STATE_SUBOBJECT *>(subObjects.data()) + index;
    }
    
    /// Add a new sub object
    /// \param type given type
    /// \return data, length must be GetSize(type)
    template<typename T = void>
    const T* DeepAdd(D3D12_STATE_SUBOBJECT_TYPE type, const T* data) {
        void* dest = allocator.AllocateArray<uint8_t>(static_cast<uint32_t>(GetSize(type)));

        /// Get serialized type
        uint64_t blobSize = SerializeOpaque(type, data, dest, nullptr);

        /// Serialize against blob
        SerializeOpaque(type, data, dest, allocator.AllocateArray<uint8_t>(static_cast<uint32_t>(blobSize)));
        
        // Add entry
        subObjects.push_back(SubObject {
            .type = type,
            .desc = dest
        });

        return static_cast<const T *>(dest);
    }
    
    /// Add a new sub object
    /// \param type given type
    /// \return data, length must be GetSize(type)
    template<typename T = void, typename = std::enable_if_t<!std::is_pointer_v<T>>>
    const T* DeepAdd(D3D12_STATE_SUBOBJECT_TYPE type, const T& data) {
        return DeepAdd(type, &data);
    }
    
    /// Add a new sub object
    /// \param type given type
    /// \return data, length must be GetSize(type)
    void Add(D3D12_STATE_SUBOBJECT_TYPE type, const void* data) {
        // Add entry
        subObjects.push_back(SubObject {
            .type = type,
            .desc = Embed(data, static_cast<uint32_t>(GetSize(type)))
        });
    }

    /// Add a new sub object
    /// \param type given type
    /// \return data, length must be GetSize(type)
    template<typename T = void, typename = std::enable_if_t<!std::is_pointer_v<T>>>
    void Add(D3D12_STATE_SUBOBJECT_TYPE type, const T& value) {
        ASSERT(GetSize(type) == sizeof(value), "Unexpected size");
        
        // Add entry
        subObjects.push_back(SubObject {
            .type = type,
            .desc = Embed(&value, static_cast<uint32_t>(GetSize(type)))
        });
    }

    /// Add a new sub object
    /// \param type given type
    /// \return ptr, inline pointer data to be copied
    void AddPtr(D3D12_STATE_SUBOBJECT_TYPE type, const void* ptr) {
        ASSERT(GetSize(type) == sizeof(ptr), "Unexpected type");
        Add(type, &ptr);
    }

    /// Get the number of subobjects
    uint64_t SubObjectCount() {
       return subObjects.size();
    }

    /// Embed data
    /// \param value value to be embedded
    /// \return embedded pointer
    template<typename T>
    T* Embed(const T& value) {
        return static_cast<T*>(Embed(&value, sizeof(T)));
    }

    /// Embed data
    /// \param data data pointer
    /// \param size byte length of data
    /// \return embedded pointer
    template<typename T = void>
    T* Embed(const T* data, uint32_t size) {
        void* dest = allocator.AllocateArray<uint8_t>(size);
        std::memcpy(dest, data, size);
        return static_cast<T*>(dest);
    }

    /// Allocate data
    /// \param size byte length of data
    /// \return allocated pointer
    template<typename T = void>
    T* Alloc(uint32_t size) {
        return reinterpret_cast<T*>(allocator.AllocateArray<uint8_t>(size));
    }

    /// Embed an ansi string, null terminated
    /// \param str string to embed
    /// \param len view length
    /// \return embedded string
    LPCWSTR EmbedAnsi(const char* str, uint64_t len) {
        auto* wide = Alloc<wchar_t>(static_cast<uint32_t>(sizeof(wchar_t) * (len + 1)));
        for (uint64_t i = 0; i < len; i++) {
            // TODO[rt]: Actually handle locale
            wide[i] = static_cast<wchar_t>(str[i]);
        }
    
        // OK
        wide[len] = 0;
        return wide;
    }

    /// Embed an ansi string, null terminated
    /// \param str string to embed
    /// \return embedded string
    LPCWSTR EmbedAnsi(const char* str) {
        if (!str || !str[0]) {
            return nullptr;
        }

        // OK
        return EmbedAnsi(str, std::strlen(str));
    }

    /// Get the description
    /// \param type state object type
    /// \return final description
    D3D12_STATE_OBJECT_DESC GetDesc(D3D12_STATE_OBJECT_TYPE type, uint32_t offset = 0) {
        ASSERT(offset < subObjects.size(), "Invalid offset");
        
        // Resolve all pending associations
        for (const PendingAssociation& association : pendingAssociations) {
            auto* object = static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION *>(subObjects[association.subObject].desc);
            object->pSubobjectToAssociate = reinterpret_cast<const D3D12_STATE_SUBOBJECT *>(&subObjects[association.associatedObject]);
        }

        D3D12_STATE_OBJECT_DESC desc;
        desc.Type = type;
        desc.NumSubobjects = static_cast<uint32_t>(subObjects.size()) - offset;
        desc.pSubobjects = reinterpret_cast<const D3D12_STATE_SUBOBJECT *>(subObjects.data()) + offset;
        return desc;
    }

    /// Get the description
    /// \return final description
    D3D12_STATE_OBJECT_DESC GetUnresolvedDesc() const {
        D3D12_STATE_OBJECT_DESC desc{};
        desc.NumSubobjects = static_cast<uint32_t>(subObjects.size());
        desc.pSubobjects = reinterpret_cast<const D3D12_STATE_SUBOBJECT *>(subObjects.data());
        return desc;
    }

    /// Add a new sub-object association, to be resolved later
    /// \param names associated names
    /// \param count number of names
    /// \param index sub-object index to associate to
    void SubObjectAssociation(const LPCWSTR* names, uint32_t count, uint32_t index) {
        // Copy over all export strings
        TrivialStackVector<LPCWSTR, 4u> exports;
        for (uint32_t i = 0; i < count; i++) {
            exports.Add(Embed<wchar_t>(names[i], static_cast<uint32_t>(sizeof(wchar_t) * (std::wcslen(names[i]) + 1))));
        }
        
        Add(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION {
            .pSubobjectToAssociate = nullptr,
            .NumExports = count,
            .pExports = Embed<LPCWSTR>(exports.Data(), sizeof(LPCWSTR) * count)
        });

        // Resolve it later
        pendingAssociations.push_back(PendingAssociation {
            .subObject = static_cast<uint32_t>(subObjects.size()) - 1,
            .associatedObject = index
        });
    }

    /// Add a new sub-object association, to be resolved later
    /// \param name associated name
    /// \param index sub-object index to associate to
    void SubObjectAssociation(LPCWSTR name, uint32_t index) {
        SubObjectAssociation(&name, 1u, index);
    }

private:
    /// Serialize a POD sub object
    /// \return additional size
    template<typename T>
    size_t SerializePOD(D3D12_STATE_SUBOBJECT_TYPE type, const void* source, void* dest) {
        ASSERT(GetSize(type) == sizeof(T), "Unexpected type");
        std::memcpy(dest, source, sizeof(T));
        return 0;
    }
    
    /// Serialize an opque type
    /// \param type chunk type
    /// \param source source data, used for serialization
    /// \param dest serialization target
    /// \param blob optional, sub-data blob
    /// \return byte size of sub-data
    size_t SerializeOpaque(D3D12_STATE_SUBOBJECT_TYPE type, const void* source, void* dest, void* blob) {
        switch (type) {
            default:
                return 0;
            case D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG:
                return Serialize(*static_cast<const D3D12_STATE_OBJECT_CONFIG *>(source), *static_cast<D3D12_STATE_OBJECT_CONFIG *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE:
                return Serialize(*static_cast<const D3D12_GLOBAL_ROOT_SIGNATURE *>(source), *static_cast<D3D12_GLOBAL_ROOT_SIGNATURE *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE:
                return Serialize(*static_cast<const D3D12_LOCAL_ROOT_SIGNATURE *>(source), *static_cast<D3D12_LOCAL_ROOT_SIGNATURE *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK:
                return Serialize(*static_cast<const D3D12_NODE_MASK *>(source), *static_cast<D3D12_NODE_MASK *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY:
                return Serialize(*static_cast<const D3D12_DXIL_LIBRARY_DESC *>(source), *static_cast<D3D12_DXIL_LIBRARY_DESC *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION:
                return Serialize(*static_cast<const D3D12_EXISTING_COLLECTION_DESC *>(source), *static_cast<D3D12_EXISTING_COLLECTION_DESC *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
                return Serialize(*static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION *>(source), *static_cast<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION:
                return Serialize(*static_cast<const D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION *>(source), *static_cast<D3D12_DXIL_SUBOBJECT_TO_EXPORTS_ASSOCIATION *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG:
                return Serialize(*static_cast<const D3D12_RAYTRACING_SHADER_CONFIG *>(source), *static_cast<D3D12_RAYTRACING_SHADER_CONFIG *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG:
                return Serialize(*static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG *>(source), *static_cast<D3D12_RAYTRACING_PIPELINE_CONFIG *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP:
                return Serialize(*static_cast<const D3D12_HIT_GROUP_DESC *>(source), *static_cast<D3D12_HIT_GROUP_DESC *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG1:
                return Serialize(*static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG1 *>(source), *static_cast<D3D12_RAYTRACING_PIPELINE_CONFIG1 *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_WORK_GRAPH:
                return Serialize(*static_cast<const D3D12_WORK_GRAPH_DESC *>(source), *static_cast<D3D12_WORK_GRAPH_DESC *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_STREAM_OUTPUT:
                return Serialize(*static_cast<const D3D12_STREAM_OUTPUT_DESC *>(source), *static_cast<D3D12_STREAM_OUTPUT_DESC *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_BLEND:
                return SerializePOD<D3D12_BLEND>(type, source, dest);
            case D3D12_STATE_SUBOBJECT_TYPE_SAMPLE_MASK:
                return SerializePOD<D3D12_SAMPLE_MASK>(type, source, dest);
            case D3D12_STATE_SUBOBJECT_TYPE_RASTERIZER:
                return Serialize(*static_cast<const D3D12_RASTERIZER_DESC *>(source), *static_cast<D3D12_RASTERIZER_DESC *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL:
                return Serialize(*static_cast<const D3D12_DEPTH_STENCIL_DESC *>(source), *static_cast<D3D12_DEPTH_STENCIL_DESC *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_INPUT_LAYOUT:
                return Serialize(*static_cast<const D3D12_INPUT_LAYOUT_DESC *>(source), *static_cast<D3D12_INPUT_LAYOUT_DESC *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_IB_STRIP_CUT_VALUE:
                return Serialize(*static_cast<const D3D12_IB_STRIP_CUT_VALUE *>(source), *static_cast<D3D12_IB_STRIP_CUT_VALUE *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_PRIMITIVE_TOPOLOGY:
                return SerializePOD<D3D12_PRIMITIVE_TOPOLOGY>(type, source, dest);
            case D3D12_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
                return Serialize(*static_cast<const D3D12_RT_FORMAT_ARRAY *>(source), *static_cast<D3D12_RT_FORMAT_ARRAY *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL_FORMAT:
                return SerializePOD<D3D12_DEPTH_STENCIL_FORMAT>(type, source, dest);
            case D3D12_STATE_SUBOBJECT_TYPE_SAMPLE_DESC:
                return SerializePOD<DXGI_SAMPLE_DESC>(type, source, dest);
            case D3D12_STATE_SUBOBJECT_TYPE_FLAGS:
                return SerializePOD<D3D12_PIPELINE_STATE_FLAGS>(type, source, dest);
            case D3D12_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
                return Serialize(*static_cast<const D3D12_DEPTH_STENCIL_DESC1 *>(source), *static_cast<D3D12_DEPTH_STENCIL_DESC1 *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING:
                return Serialize(*static_cast<const D3D12_VIEW_INSTANCING_DESC *>(source), *static_cast<D3D12_VIEW_INSTANCING_DESC *>(dest), blob);
            case D3D12_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL2:
                return Serialize(*static_cast<const D3D12_DEPTH_STENCIL_DESC2 *>(source), *static_cast<D3D12_DEPTH_STENCIL_DESC2 *>(dest), blob);
        }
    }

private:
    struct PendingAssociation {
        uint32_t subObject        = 0;
        uint32_t associatedObject = 0;
    };

private:
    struct SubObject {
        D3D12_STATE_SUBOBJECT_TYPE type;
        void *desc{nullptr};
    };
    
    /// All pending entries
    Vector<SubObject> subObjects;

    /// All pending associations
    Vector<PendingAssociation> pendingAssociations;

    /// Internal allocator
    LinearBlockAllocator<4096> allocator;
};
