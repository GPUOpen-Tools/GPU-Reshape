// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

// Backend
#include "Variable.h"
#include "IdentifierMap.h"
#include "TypeMap.h"
#include "TypeFormat.h"
#include "Variable.h"
#include <Backend/ShaderData/ShaderDataInfo.h>

// Std
#include <unordered_map>

namespace IL {
    struct ShaderDataMap {
        using Container = std::vector<ShaderDataInfo>;

        ShaderDataMap(IdentifierMap& identifierMap, Backend::IL::TypeMap& typeMap) : identifierMap(identifierMap), typeMap(typeMap) {

        }

        /// Add a new data
        /// \param info data information
        void Add(const ShaderDataInfo& info) {
            datas.push_back(info);

            // Handle type
            switch (info.type) {
                default:
                    ASSERT(false, "Invalid data");
                    break;
                case ShaderDataType::Buffer:
                    Add(info.id, typeMap.FindTypeOrAdd(Backend::IL::PointerType{
                        .pointee = typeMap.FindTypeOrAdd(Backend::IL::BufferType{
                            .elementType = Backend::IL::GetSampledFormatType(typeMap, info.buffer.format),
                            .samplerMode = Backend::IL::ResourceSamplerMode::Writable,
                            .texelType = info.buffer.format
                        }),
                        .addressSpace = Backend::IL::AddressSpace::Resource
                    }));
                    break;
                case ShaderDataType::Texture:
                    ASSERT(false, "Not implemented");
                    break;
                case ShaderDataType::Event:
                    Add(info.id, typeMap.FindTypeOrAdd(Backend::IL::IntType {
                        .bitWidth = 32,
                        .signedness = false
                    }));
                    break;
                case ShaderDataType::Descriptor:
                    Add(info.id, typeMap.FindTypeOrAdd(Backend::IL::IntType {
                        .bitWidth = 32,
                        .signedness = false
                    }));
                    break;
            }
        }

        /// Get the variable from a data id
        /// \param rid allocated identifier
        /// \return nullptr if not found
        const Backend::IL::Variable* Get(ShaderDataID rid) {
            auto it = identifiers.find(rid);
            if (it == identifiers.end()) {
                return nullptr;
            }

            return &it->second.variable;
        }

        /// Iterator accessors
        Container::iterator begin() { return datas.begin(); }
        Container::reverse_iterator rbegin() { return datas.rbegin(); }
        Container::iterator end() { return datas.end(); }
        Container::reverse_iterator rend() { return datas.rend(); }
        Container::const_iterator begin() const { return datas.begin(); }
        Container::const_reverse_iterator rbegin() const { return datas.rbegin(); }
        Container::const_iterator end() const { return datas.end(); }
        Container::const_reverse_iterator rend() const { return datas.rend(); }

        /// Get the number of datas
        /// \return
        uint32_t GetCount() {
            return static_cast<uint32_t>(datas.size());
        }

    private:
        /// Add a data
        /// \param rid allocation index
        /// \param type expected type
        void Add(ShaderDataID rid, const Backend::IL::Type* type) {
            ID id = identifierMap.AllocID();

            // Set the type association
            typeMap.SetType(id, type);

            // Set RID to ID lookup
            DataEntry& entry = identifiers[rid];
            entry.variable.id = id;
            entry.variable.type = type;
            entry.variable.addressSpace = Backend::IL::AddressSpace::Resource;
        }

    private:
        struct DataEntry {
            Backend::IL::Variable variable;
        };

    private:
        /// The shared identifier map
        IdentifierMap& identifierMap;

        /// The shared type map
        Backend::IL::TypeMap& typeMap;

        /// Data mappings
        std::unordered_map<ShaderDataID, DataEntry> identifiers;

        /// All datas
        Container datas;
    };
}
