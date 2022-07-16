#pragma once

// Backend
#include "Type.h"
#include "ID.h"

// Common
#include <Common/Containers/LinearBlockAllocator.h>

// Backend
#include "IdentifierMap.h"

// Std
#include <map>

namespace Backend::IL {
    using namespace ::IL;
    
    /// Type map, provides unique type identifiers 
    struct TypeMap {
        using Container = std::vector<Type*>;

        TypeMap(const Allocators &allocators, IdentifierMap& identifierMap) : allocators(allocators), blockAllocator(allocators), identifierMap(identifierMap) {

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
                typePtr = AllocateType<T>(identifierMap.AllocID(), type);
            }

            return typePtr;
        }

        /// Add a type to this map, must be unique
        /// \param type the type to be added
        template<typename T>
        const T* AddType(ID id, const T &type) {
            auto&& sortMap = GetSortMap<T>();

            auto &typePtr = sortMap[type.SortKey()];
            if (!typePtr) {
                typePtr = AllocateType<T>(id, type);
            }

            return typePtr;
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
            idMap[id] = type;
        }

        /// Get the type for a given id
        /// \param id the id to be looked up
        /// \return the resulting type, may be nullptr
        const Type *GetType(ID id) const {
            auto it = idMap.find(id);
            if (it == idMap.end()) {
                return nullptr;
            }

            return it->second;
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

    private:
        /// Allocate a new type
        /// \tparam T the type cxx type
        /// \param decl the declaration specifier
        /// \return the allocated type
        template<typename T>
        T *AllocateType(ID id, const T &decl) {
            auto *type = blockAllocator.Allocate<T>(decl);
            type->kind = T::kKind;
            type->id = id;
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
        template<> SortMap<FunctionType>& GetSortMap<FunctionType>() { return maps.functionMap; }

    private:
        Allocators allocators;

        /// Block allocator for types, types never need to be freed
        LinearBlockAllocator<1024> blockAllocator;

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
            SortMap<FunctionType> functionMap;
        };

        /// Declaration order
        Container types;

        /// Identifiers
        IdentifierMap& identifierMap;

        TypeMaps maps;

        /// Id lookup
        std::map<ID, const Type *> idMap;
    };
}
