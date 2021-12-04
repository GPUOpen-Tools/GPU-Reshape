#pragma once

#include <string>
#include <map>

/// Simple cxx type info
struct TypeInfo {
    bool operator==(const TypeInfo& other) const {
        return cxxType == other.cxxType && size == other.size;
    }

    bool operator!=(const TypeInfo& other) const {
        return cxxType != other.cxxType || size != other.size;
    }

    std::string cxxType;
    uint64_t size{0};
};

/// Supported cxx primitives
struct PrimitiveTypeMap {
    PrimitiveTypeMap();

    std::map<std::string, TypeInfo> types;
};