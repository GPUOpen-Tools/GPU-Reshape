#pragma once

// Backend
#include <Backend/IL/TypeMap.h>
#include <Backend/IL/IdentifierMap.h>

// Std
#include <vector>

class DXILTypeMap {
public:
    DXILTypeMap(Backend::IL::TypeMap& programMap, Backend::IL::IdentifierMap& identifierMap) : programMap(programMap), identifierMap(identifierMap) {

    }

    /// Set the number of entries
    void SetEntryCount(uint32_t count) {
        indexLookup.resize(count);
    }

    /// Copy this type map
    /// \param out destination map
    void CopyTo(DXILTypeMap& out) {
        out.indexLookup = indexLookup;
        out.typeLookup = typeLookup;
    }

    /// Add a type
    /// \param index the linear block index
    /// \param decl the IL type declaration
    /// \return the new type
    template<typename T>
    const T* AddType(uint32_t index, const T& decl) {
        // LLVM types are indexed separately from global identifiers, so always allocate
        const T* type = programMap.AddType<T>(identifierMap.AllocID(), decl);
        indexLookup.at(index) = type;
        AddTypeMapping(type, index);
        return type;
    }

    /// Add a type
    /// \param index the linear block index
    /// \param decl the IL type declaration
    /// \return the new type
    template<typename T>
    const T* AddUnsortedType(uint32_t index, const T& decl) {
        // LLVM types are indexed separately from global identifiers, so always allocate
        const T* type = programMap.AddUnsortedType<T>(identifierMap.AllocID(), decl);
        indexLookup.at(index) = type;
        AddTypeMapping(type, index);
        return type;
    }

    /// Get a type
    /// \param index the linear record type index
    /// \return may be nullptr
    const Backend::IL::Type* GetType(uint32_t index) {
        return indexLookup.at(index);
    }

    /// Get a type
    /// \param index the linear record type index
    /// \return may be nullptr
    uint32_t GetType(const Backend::IL::Type* type) {
        return typeLookup.at(type->id);
    }

    /// Add a type mapping from IL to DXIL
    /// \param type IL type
    /// \param index DXIL index
    void AddTypeMapping(const Backend::IL::Type* type, uint32_t index) {
        if (typeLookup.size() <= type->id) {
            typeLookup.resize(type->id + 1, ~0u);
        }

        typeLookup[type->id] = index;
    }

    /// Check if there is an existing type mapping
    /// \param type IL type
    /// \return true if present
    bool HasType(const Backend::IL::Type* type) {
        return type->id < typeLookup.size() && typeLookup[type->id] != ~0u;
    }

private:
    /// IL map
    Backend::IL::TypeMap& programMap;

    /// Identifier map
    Backend::IL::IdentifierMap& identifierMap;

    /// Local lookup table
    std::vector<const Backend::IL::Type*> indexLookup;

    /// IL type to DXIL type table
    std::vector<uint32_t> typeLookup;
};
