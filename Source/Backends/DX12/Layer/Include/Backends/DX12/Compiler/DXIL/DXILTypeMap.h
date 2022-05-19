#pragma once

// Backend
#include <Backend/IL/TypeMap.h>

// Std
#include <vector>

class DXILTypeMap {
public:
    DXILTypeMap(Backend::IL::TypeMap* programMap) : programMap(programMap) {

    }

    /// Set the number of entries
    void SetEntryCount(uint32_t count) {
        indexLookup.resize(count);
    }

    /// Add a type
    /// \param index the linear block index
    /// \param decl the IL type declaration
    /// \return the new type
    template<typename T>
    const Backend::IL::Type* AddType(uint32_t index, const T& decl) {
        const Backend::IL::Type* type = programMap->AddType<T>(decl);
        indexLookup.at(index) = type;
        return type;
    }

    /// Get a type
    /// \param index the linear record type index
    /// \return may be nullptr
    const Backend::IL::Type* GetType(uint32_t index) {
        return indexLookup.at(index);
    }

private:
    /// IL map
    Backend::IL::TypeMap* programMap;

    /// Local lookup table
    std::vector<const Backend::IL::Type*> indexLookup;
};
