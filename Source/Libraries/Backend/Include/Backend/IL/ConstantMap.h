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

// Backend
#include "Constant.h"
#include "TypeMap.h"
#include "IdentifierMap.h"

// Common
#include <Common/Containers/LinearBlockAllocator.h>

// Std
#include <unordered_map>

namespace Backend::IL {
    using namespace ::IL;

    /// Constant map, provides unique constants
    struct ConstantMap {
        using Container = std::vector<Constant*>;
        
        ConstantMap(const Allocators &allocators, IdentifierMap& identifierMap, TypeMap& typeMap, const CapabilityTable& capabilityTable)
            : allocators(allocators), blockAllocator(allocators), capabilityTable(capabilityTable), identifierMap(identifierMap), typeMap(typeMap) {

        }

        /// Create a copy of this constant map
        /// \param out destination map
        void CopyTo(ConstantMap& out) const {
            // Copy the maps
            out.idMap = idMap;
            out.maps = maps;
            out.constants = constants;
        }

        /// Find a constant from his map
        /// \param constant the constant declaration
        /// \return the constant pointer, nullptr if not found
        template<typename T>
        const T* FindConstant(const typename T::Type* type, const T &constant) {
            auto&& sortMap = GetSortMap<T>();

            if (auto it = sortMap.find(constant.SortKey(type)); it != sortMap.end()) {
                return it->second;
            }

            return nullptr;
        }

        /// Find a constant from this map, or create a new one
        /// \param constant the constant declaration
        /// \return the constant pointer
        template<typename T>
        const T* FindConstantOrAdd(const typename T::Type* type, const T &constant) {
            auto&& sortMap = GetSortMap<T>();

            auto &constantPtr = sortMap[constant.SortKey(type)];
            if (!constantPtr) {
                constantPtr = AllocateConstant<T>(identifierMap.AllocID(), type, constant);
            }

            return constantPtr;
        }

        /// Add a constant to this map, must be unique
        /// \param constant the constant to be added
        template<typename T>
        const Constant* AddConstant(ID id, const typename T::Type* type, const T &constant) {
            auto&& sortMap = GetSortMap<T>();

            auto &constantPtr = sortMap[constant.SortKey(type)];
            if (!constantPtr) {
                constantPtr = AllocateConstant<T>(id, type, constant);
                idMap[id] = constantPtr;
            }

            return constantPtr;
        }

        /// Add a constant to this map, must be unique
        /// \param constant the constant to be added
        template<typename T>
        const Constant* AddUnsortedConstant(ID id, const Backend::IL::Type* type, const T &constant) {
            auto constantPtr = AllocateConstant<T>(id, type, constant);
            idMap[id] = constantPtr;
            return constantPtr;
        }

        /// Set a constant relation in this map
        /// \param id the id to be associated
        /// \param constant the resulting constant
        void SetConstant(ID id, const Constant *constant) {
            ASSERT(id != InvalidID, "SetConstant must have a valid id");
            idMap[id] = constant;
        }

        /// Get the constant for a given id
        /// \param id the id to be looked up
        /// \return the resulting constant, may be nullptr
        const Constant *GetConstant(ID id) {
            const Constant *constant = idMap[id];
            return constant;
        }

        /// Get the constant for a given id
        /// \param id the id to be looked up
        /// \return the resulting constant, may be nullptr
        template<typename T>
        const T *GetConstant(ID id) {
            const Constant *constant = idMap[id];
            return constant ? constant->Cast<T>() : nullptr;
        }

        /// Get the type map
        TypeMap& GetTypeMap() {
            return typeMap;
        }

        /// Get the type map
        const TypeMap& GetTypeMap() const {
            return typeMap;
        }

        /// Iterator accessors
        Container::iterator begin() { return constants.begin(); }
        Container::reverse_iterator rbegin() { return constants.rbegin(); }
        Container::iterator end() { return constants.end(); }
        Container::reverse_iterator rend() { return constants.rend(); }
        Container::const_iterator begin() const { return constants.begin(); }
        Container::const_reverse_iterator rbegin() const { return constants.rbegin(); }
        Container::const_iterator end() const { return constants.end(); }
        Container::const_reverse_iterator rend() const { return constants.rend(); }

    private:
        /// Allocate a new constant
        /// \tparam T the constant cxx constant
        /// \param decl the declaration specifier
        /// \return the allocated constant
        template<typename T>
        T *AllocateConstant(ID id, const Backend::IL::Type* type, const T &decl) {
            typeMap.SetType(id, type);

            auto *constant = blockAllocator.Allocate<T>(decl);
            constant->id = id;
            constant->type = type;
            constant->kind = T::kKind;
            constants.push_back(constant);
            return constant;
        }

        template<typename T>
        using SortMap = std::map<ConstantSortKey<T>, T*>;

        /// Map fetchers
        template<typename T>
        SortMap<T>& GetSortMap() {}

        /// Map fetcher impl
        template<> SortMap<UnexposedConstant>& GetSortMap<UnexposedConstant>() { return maps.unexposedMap; }
        template<> SortMap<BoolConstant>& GetSortMap<BoolConstant>() { return maps.boolMap; }
        template<> SortMap<IntConstant>& GetSortMap<IntConstant>() { return maps.intMap; }
        template<> SortMap<FPConstant>& GetSortMap<FPConstant>() { return maps.fpMap; }
        template<> SortMap<StructConstant>& GetSortMap<StructConstant>() { return maps.structMap; }
        template<> SortMap<UndefConstant>& GetSortMap<UndefConstant>() { return maps.undefMap; }
        template<> SortMap<NullConstant>& GetSortMap<NullConstant>() { return maps.nullMap; }

    private:
        Allocators allocators;

        /// Block allocator for constants, constants never need to be freed
        LinearBlockAllocator<1024> blockAllocator;

        /// Unique constraints for type mapping
        const CapabilityTable& capabilityTable;

        /// Constant cache
        struct ConstantMaps {
            SortMap<UnexposedConstant> unexposedMap;
            SortMap<BoolConstant> boolMap;
            SortMap<IntConstant> intMap;
            SortMap<FPConstant> fpMap;
            SortMap<StructConstant> structMap;
            SortMap<UndefConstant> undefMap;
            SortMap<NullConstant> nullMap;
        };
        
        /// Declaration order
        std::vector<Constant*> constants;

        /// Identifiers
        IdentifierMap& identifierMap;

        /// Types
        TypeMap& typeMap;

        /// All maps
        ConstantMaps maps;

        /// Id lookup
        std::unordered_map<ID, const Constant *> idMap;
    };
}
