#pragma once

// Generator
#include "Attribute.h"

// Common
#include <Common/String.h>

// Std
#include <vector>
#include <algorithm>

/// Map of attributes
struct AttributeMap {
    /// Check if this map contains an attribute
    /// \param name the name of the given attribute
    /// \return does the attribute exist
    bool Has(const std::string& name) const {
        return std::any_of(attributes.begin(), attributes.end(), [&](const Attribute& attribute) {
            return attribute.name == name;
        });
    }

    /// Get an attribute from this map
    /// \param name name of the attribute
    /// \return nullptr if not found
    const Attribute* Get(const std::string& name) const {
        auto it = std::find_if(attributes.begin(), attributes.end(), [&](const Attribute& attribute) {
            return attribute.name == name;
        });

        if (it == attributes.end()) {
            return nullptr;
        }

        return &*it;
    }

    /// Get a value from this attribute map
    /// \param name the attribute name
    /// \param _default returned value if not found
    /// \return attribute value
    const std::string& GetValue(const std::string& name, const std::string& _default) const {
        auto* attr = Get(name);
        if  (!attr) {
            return _default;
        }

        return attr->value;
    }

    /// Get a boolean value from this attribute map
    /// \param name the attribute name
    /// \param _default returned value if not found
    /// \return boolean attribute value
    bool GetBool(const std::string& name, bool _default = false) const {
        auto* attr = Get(name);
        if  (!attr) {
            return _default;
        }

        return std::iequals(attr->value, "true");
    }

    /// Add an attribute to this map
    /// \param name name of the attribute
    /// \param value value of the attribute
    void Add(const std::string& name, const std::string& value) {
        Attribute& attr = attributes.emplace_back();
        attr.name = name;
        attr.value = value;
    }

    /// All attributes
    std::vector<Attribute> attributes;
};
