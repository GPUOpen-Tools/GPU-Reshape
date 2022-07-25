#pragma once

// Backend
#include <Backend/IL/ConstantMap.h>
#include <Backend/IL/IdentifierMap.h>

// Std
#include <vector>

class DXILConstantMap {
public:
    DXILConstantMap(Backend::IL::ConstantMap& programMap, Backend::IL::IdentifierMap& identifierMap) : programMap(programMap), identifierMap(identifierMap) {

    }

    /// Copy this constant map
    /// \param out destination map
    void CopyTo(DXILConstantMap& out) {
        out.constants = constants;
        out.typeLookup = typeLookup;
    }

    /// Add a constant to this map, must be unique
    /// \param constant the constant to be added
    template<typename T>
    const Backend::IL::Constant* AddConstant(IL::ID id, const typename T::Type* type, const T &constant) {
        const auto *constantPtr = programMap.AddConstant<T>(id, type, constant);
        constants.push_back(constantPtr);
        AddConstantMapping(constantPtr, id);
        return constantPtr;
    }

    /// Add a constant to this map, must be unique
    /// \param constant the constant to be added
    template<typename T>
    const Backend::IL::Constant* AddUnsortedConstant(IL::ID id, const Backend::IL::Type* type, const T &constant) {
        const auto *constantPtr = programMap.AddUnsortedConstant<T>(id, type, constant);
        constants.push_back(constantPtr);
        AddConstantMapping(constantPtr, id);
        return constantPtr;
    }

    /// Get constant at offset
    /// \param id source id
    /// \return nullptr if not found
    const IL::Constant* GetConstant(uint32_t id) {
        if (id >= constants.size()) {
            return nullptr;
        }

        return constants[id];
    }

    /// Get a constant from a type
    /// \param type IL type
    /// \return DXIL id
    uint32_t GetConstant(const Backend::IL::Constant* type) {
        return typeLookup.at(type->id);
    }

    /// Add a new constant mapping
    /// \param type IL type
    /// \param index DXIL id
    void AddConstantMapping(const Backend::IL::Constant* type, uint32_t index) {
        if (typeLookup.size() <= type->id) {
            typeLookup.resize(type->id + 1, ~0u);
        }

        typeLookup[type->id] = index;
    }

    /// Check if a constant is present in DXIL
    /// \param type IL type
    /// \return true if present
    bool HasConstant(const Backend::IL::Constant* type) {
        return type->id < typeLookup.size() && typeLookup[type->id] != ~0u;
    }

private:
    /// IL map
    Backend::IL::ConstantMap& programMap;

    /// Identifier map
    Backend::IL::IdentifierMap& identifierMap;

    /// All constants
    std::vector<const IL::Constant*> constants;

    /// IL type to DXIL type table
    std::vector<uint32_t> typeLookup;
};
