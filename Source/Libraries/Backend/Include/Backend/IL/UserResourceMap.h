#pragma once

// Backend
#include "Variable.h"
#include "IdentifierMap.h"
#include "TypeMap.h"
#include "TypeFormat.h"
#include "Variable.h"
#include <Backend/Resource/ShaderResourceInfo.h>

// Std
#include <map>

namespace IL {
    struct UserResourceMap {
        using Container = std::vector<ShaderResourceInfo>;
        
        UserResourceMap(IdentifierMap& identifierMap, Backend::IL::TypeMap& typeMap) : identifierMap(identifierMap), typeMap(typeMap) {

        }

        /// Add a new resource
        /// \param info resource information
        void Add(const ShaderResourceInfo& info) {
            resources.push_back(info);

            // Handle type
            switch (info.type) {
                default:
                    ASSERT(false, "Invalid resource");
                    break;
                case ShaderResourceType::Buffer:
                    Add(info.id, typeMap.FindTypeOrAdd(Backend::IL::BufferType {
                        .elementType = Backend::IL::GetSampledFormatType(typeMap, info.buffer.format),
                        .texelType = info.buffer.format
                    }));
                    break;
                case ShaderResourceType::Texture:
                    ASSERT(false, "Not implemented");
                    break;
            }
        }

        /// Get the resource from a resource id
        /// \param rid allocated identifier
        /// \return nullptr if not found
        const Backend::IL::Variable* Get(ShaderResourceID rid) {
            auto it = identifiers.find(rid);
            if (it == identifiers.end()) {
                return nullptr;
            }

            return &it->second.variable;
        }

        /// Iterator accessors
        Container::iterator begin() { return resources.begin(); }
        Container::reverse_iterator rbegin() { return resources.rbegin(); }
        Container::iterator end() { return resources.end(); }
        Container::reverse_iterator rend() { return resources.rend(); }
        Container::const_iterator begin() const { return resources.begin(); }
        Container::const_reverse_iterator rbegin() const { return resources.rbegin(); }
        Container::const_iterator end() const { return resources.end(); }
        Container::const_reverse_iterator rend() const { return resources.rend(); }

        /// Get the number of resources
        /// \return
        uint32_t GetCount() {
            return resources.size();
        }

    private:
        /// Add a resource
        /// \param rid allocation index
        /// \param type expected type
        void Add(ShaderResourceID rid, const Backend::IL::Type* type) {
            ID id = identifierMap.AllocID();

            // Set the type association
            typeMap.SetType(id, type);

            // Set RID to ID lookup
            ResourceEntry& entry = identifiers[rid];
            entry.variable.id = id;
            entry.variable.type = type;
            entry.variable.addressSpace = Backend::IL::AddressSpace::Resource;
        }

    private:
        struct ResourceEntry {
            Backend::IL::Variable variable;
        };

    private:
        /// The shared identifier map
        IdentifierMap& identifierMap;

        /// The shared type map
        Backend::IL::TypeMap& typeMap;

        /// Resource mappings
        std::map<ShaderResourceID, ResourceEntry> identifiers;

        /// All resources
        Container resources;
    };
}
