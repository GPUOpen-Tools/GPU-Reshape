#pragma once

// Json
#include <nlohmann/json.hpp>

// Std
#include <sstream>
#include <iostream>

/// Pretty print a type
/// \param ss output stream
/// \param type the json type
/// \param isFPtr outer function pointer?
/// \return success state
static bool PrettyPrintType(std::stringstream &ss, const nlohmann::json &type, bool isFPtr = false) {
    auto typeKind = type["type"].get<std::string>();

    // Common
    bool isPointer = typeKind == "pointer";
    bool isConst   = type["const"].get<int>();

    // Handle qualifiers
    if (!isPointer && isConst) {
        ss << "const ";
    }

    // Handle kind
    if (typeKind == "pod") {
        ss << type["name"].get<std::string>();
    } else if (typeKind == "void") {
        ss << "void";
    } else if (typeKind == "lref") {
        if (!PrettyPrintType(ss, type["contained"])) {
            return false;
        }

        ss << "&";
    } else if (typeKind == "rref") {
        if (!PrettyPrintType(ss, type["contained"])) {
            return false;
        }

        ss << "&&";
    } else if (typeKind == "array") {
        if (!PrettyPrintType(ss, type["contained"])) {
            return false;
        }

        ss << "[" << type["size"] << "]";
    } else if (isPointer) {
        bool isFunction = type["contained"]["type"] == "function";

        if (!PrettyPrintType(ss, type["contained"], isFunction)) {
            return false;
        }

        if (!isFunction) {
            ss << "*";
        }

        if (isConst) {
            ss << " const";
        }
    } else if (typeKind == "function") {
        if (!PrettyPrintType(ss, type["returnType"])) {
            return false;
        }

        ss << "(";

        if (isFPtr) {
            ss << "*";
        }

        ss << ")(";

        auto &&parameters = type["parameters"];

        for (auto it = parameters.begin(); it != parameters.end(); it++) {
            if (it != parameters.begin()) {
                ss << ", ";
            }

            if (!PrettyPrintType(ss, *it)) {
                return false;
            }
        }

        ss << ")";
    } else {
        std::cerr << "Unexpected type '" << typeKind << "'\n";
        return false;
    }

    // OK
    return true;
}

/// Pretty print a parameterized function
/// \param ss output stream
/// \param type the json type
/// \param name parameter name
/// \param top top type?
/// \return success state
static bool PrettyPrintParameter(std::stringstream &ss, const nlohmann::json &type, const std::string &name, bool top = true) {
    auto typeKind = type["type"].get<std::string>();

    bool isPointer = typeKind == "pointer";
    bool isConst = type["const"].get<int>();

    // Handle qualifiers
    if (!isPointer && isConst) {
        ss << "const ";
    }

    // Handle kind
    if (typeKind == "pod") {
        ss << type["name"].get<std::string>();
    } else if (typeKind == "void") {
        ss << "void";
    } else if (typeKind == "lref") {
        if (!PrettyPrintParameter(ss, type["contained"], name, false)) {
            return false;
        }

        ss << "&";
    } else if (typeKind == "rref") {
        if (!PrettyPrintParameter(ss, type["contained"], name, false)) {
            return false;
        }

        ss << "&&";
    } else if (typeKind == "array") {
        if (!PrettyPrintParameter(ss, type["contained"], name, false)) {
            return false;
        }

        if (top) {
            ss << " " << name << "[" << type["size"] << "]";
            return true;
        } else {
            ss << "[" << type["size"] << "]";
        }
    } else if (isPointer) {
        if (!PrettyPrintParameter(ss, type["contained"], name, false)) {
            return false;
        }

        ss << "*";

        if (isConst) {
            ss << " const";
        }
    } else {
        std::cerr << "Unexpected type '" << typeKind << "'\n";
        return false;
    }

    if (top) {
        ss << " " << name;
    }

    // OK
    return true;
}

/// Get the base name of an interface
/// \param name top name
/// \return interface base name
static std::string GetInterfaceBaseName(const std::string &name) {
    size_t i = name.length() - 1;
    while (isdigit(name[i])) {
        i--;
    }

    return name.substr(0, i + 1);
}

/// Get the revision name
/// \param name base name
/// \param revision revision number
/// \return name
static std::string GetInterfaceRevision(const std::string &name, uint32_t revision) {
    return revision ? name + std::to_string(revision) : name;
}

/// Get the outer revision
/// \param info generation info
/// \param key base key
/// \return revision name
static std::string GetOuterRevision(const GeneratorInfo &info, const std::string& key) {
    uint32_t i = 0;
    while (info.specification["interfaces"].contains(key + std::to_string(i + 1))) {
        i++;
    }

    return i ? (key + std::to_string(i)) : key;
}
