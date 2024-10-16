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
#include "Type.h"
#include "ID.h"
#include "CapabilityTable.h"
#include "ResourceTokenMetadataField.h"

// Common
#include <Common/Containers/LinearBlockAllocator.h>

// Backend
#include "IdentifierMap.h"

// Std
#include <map>
#include <unordered_map>

namespace Backend::IL {
    using namespace ::IL;
    
    /// Type map, provides unique type identifiers 
    struct TypeMap {
        using Container = std::vector<Type*>;

        TypeMap(const Allocators &allocators, IdentifierMap& identifierMap, const CapabilityTable& capabilityTable)
            : allocators(allocators), blockAllocator(allocators), capabilityTable(capabilityTable), identifierMap(identifierMap) {

        }

        /// Create a copy of this type map
        ///   ! Parent lifetime tied to the copy
        /// \return the new type map
        void CopyTo(TypeMap& out) const {
            // Copy the maps
            out.idMap = idMap;
            out.maps = maps;
            out.types = types;
        }

        /// Find a type from his map
        /// \param type the type declaration
        /// \return the type pointer, nullptr if not found
        template<typename T>
        const T* FindType(const T &type) {
            auto&& sortMap = GetSortMap<T>();

            if (auto it = sortMap.find(type.SortKey()); it != sortMap.end()) {
                return it->second;
            }

            return nullptr;
        }

        /// Find a type from this map, or create a new one
        /// \param type the type declaration
        /// \return the type pointer
        template<typename T>
        const T* FindTypeOrAdd(const T &type) {
            auto&& sortMap = GetSortMap<T>();

            auto &typePtr = sortMap[type.SortKey()];
            if (!typePtr) {
                typePtr = AllocateType<T>(identifierMap.AllocID(), InvalidOffset, type);
            }

            return typePtr;
        }

        /// Add a type to this map, must be unique
        /// \param type the type to be added
        template<typename T>
        const T* AddType(ID id, const T &type) {
            auto&& sortMap = GetSortMap<T>();

            T* allocation = AllocateType<T>(id, InvalidOffset, type);

            if (auto &typePtr = sortMap[type.SortKey()]; !typePtr) {
                typePtr = allocation;
            }

            return allocation;
        }

        /// Add a type to this map, must be unique
        /// \param type the type to be added
        template<typename T>
        const T* AddType(ID id, uint32_t sourceOffset, const T &type) {
            auto&& sortMap = GetSortMap<T>();

            T* allocation = AllocateType<T>(id, sourceOffset, type);

            if (auto &typePtr = sortMap[type.SortKey()]; !typePtr) {
                typePtr = allocation;
            }

            return allocation;
        }

        /// Add an undeclared type to this map
        /// \param decl the type to be added
        template<typename T>
        const T* AddUnsortedType(ID id, const T &decl) {
            auto *type = blockAllocator.Allocate<T>(decl);
            type->kind = T::kKind;
            type->id = id;
            types.push_back(type);
            return type;
        }

        /// Set a type relation in this map
        /// \param id the id to be associated
        /// \param type the resulting type
        void SetType(ID id, const Type *type) {
            ASSERT(id != InvalidID, "SetType must have a valid id");
            ASSERT(type, "SetType must have a valid type");

            if (idMap.size() <= id) {
                idMap.resize(id + 1);
            }
            
            idMap[id] = type;
        }

        /// Get the type for a given id
        /// \param id the id to be looked up
        /// \return the resulting type, may be nullptr
        const Type *GetType(ID id) const {
            if (idMap.size() <= id) {
                return nullptr;
            }

            return idMap[id];
        }

        /// Remove a type mapping
        /// \param id the id from which to remove the type
        void RemoveType(ID id) {
            if (idMap.size() <= id) {
                return;
            }

            idMap[id] = nullptr;
        }

        /// Iterator accessors
        Container::iterator begin() { return types.begin(); }
        Container::reverse_iterator rbegin() { return types.rbegin(); }
        Container::iterator end() { return types.end(); }
        Container::reverse_iterator rend() { return types.rend(); }
        Container::const_iterator begin() const { return types.begin(); }
        Container::const_reverse_iterator rbegin() const { return types.rbegin(); }
        Container::const_iterator end() const { return types.end(); }
        Container::const_reverse_iterator rend() const { return types.rend(); }

    public: /** Type helpers */
        /// Get the inbuilt resource token type
        const StructType* GetResourceToken() {
            if (inbuilt.resourceToken) {
                return inbuilt.resourceToken;
            }

            // Filled with dwords
            const IntType *uint32 = FindTypeOrAdd(IntType{ .bitWidth = 32, .signedness = false });

            // Create struct declaration, one dword per field
            StructType decl;
            for (uint32_t i = 0; i < static_cast<uint32_t>(ResourceTokenMetadataField::Count); i++) {
                decl.memberTypes.push_back(uint32);
            }

            // Create inbuilt
            return inbuilt.resourceToken = FindTypeOrAdd(decl);
        }

    private:
        /// Allocate a new type
        /// \tparam T the type cxx type
        /// \param decl the declaration specifier
        /// \return the allocated type
        template<typename T>
        T *AllocateType(ID id, uint32_t sourceOffset, const T &decl) {
            auto *type = blockAllocator.Allocate<T>(decl);
            type->kind = T::kKind;
            type->id = id;
            type->sourceOffset = sourceOffset;
            types.push_back(type);
            return type;
        }

        template<typename T>
        using SortMap = std::map<SortKey<T>, T*>;

        /// Map fetchers
        template<typename T>
        SortMap<T>& GetSortMap() {}

        /// Map fetcher impl
        template<> SortMap<UnexposedType>& GetSortMap<UnexposedType>() { return maps.unexposedMap; }
        template<> SortMap<BoolType>& GetSortMap<BoolType>() { return maps.boolMap; }
        template<> SortMap<VoidType>& GetSortMap<VoidType>() { return maps.voidMap; }
        template<> SortMap<IntType>& GetSortMap<IntType>() { return maps.intMap; }
        template<> SortMap<FPType>& GetSortMap<FPType>() { return maps.fpMap; }
        template<> SortMap<VectorType>& GetSortMap<VectorType>() { return maps.vectorMap; }
        template<> SortMap<MatrixType>& GetSortMap<MatrixType>() { return maps.matrixMap; }
        template<> SortMap<PointerType>& GetSortMap<PointerType>() { return maps.pointerMap; }
        template<> SortMap<ArrayType>& GetSortMap<ArrayType>() { return maps.arrayMap; }
        template<> SortMap<TextureType>& GetSortMap<TextureType>() { return maps.textureMap; }
        template<> SortMap<BufferType>& GetSortMap<BufferType>() { return maps.bufferMap; }
        template<> SortMap<CBufferType>& GetSortMap<CBufferType>() { return maps.cbufferMap; }
        template<> SortMap<SamplerType>& GetSortMap<SamplerType>() { return maps.samplerMap; }
        template<> SortMap<FunctionType>& GetSortMap<FunctionType>() { return maps.functionMap; }
        template<> SortMap<StructType>& GetSortMap<StructType>() { return maps.structMap; }

    private:
        Allocators allocators;

        /// Block allocator for types, types never need to be freed
        LinearBlockAllocator<1024> blockAllocator;

        /// Unique constraints for type mapping
        const CapabilityTable& capabilityTable;

        /// Type cache
        struct TypeMaps {
            SortMap<UnexposedType> unexposedMap;
            SortMap<BoolType> boolMap;
            SortMap<VoidType> voidMap;
            SortMap<IntType> intMap;
            SortMap<FPType> fpMap;
            SortMap<VectorType> vectorMap;
            SortMap<MatrixType> matrixMap;
            SortMap<PointerType> pointerMap;
            SortMap<ArrayType> arrayMap;
            SortMap<TextureType> textureMap;
            SortMap<BufferType> bufferMap;
            SortMap<CBufferType> cbufferMap;
            SortMap<SamplerType> samplerMap;
            SortMap<FunctionType> functionMap;
            SortMap<StructType> structMap;
        } maps;

        /// Inbuilt types
        struct InbuiltTypes {
            const StructType* resourceToken{nullptr};
        } inbuilt;

        /// Declaration order
        Container types;

        /// Identifiers
        IdentifierMap& identifierMap;

        /// Id lookup
        std::vector<const Type*> idMap;
    };
}
