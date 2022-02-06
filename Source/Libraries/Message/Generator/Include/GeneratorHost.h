#pragma once

// Common
#include <Common/ComRef.h>

// Std
#include <vector>
#include <string>
#include <map>

class IGenerator;

/// Hosts a set of generators
struct GeneratorHost {
    /// Add a new schema generator
    void AddSchema(const ComRef<IGenerator>& generator) {
        schemaGenerators.push_back(generator);
    }

    /// Add a new message generator
    void AddMessage(const std::string& name, const ComRef<IGenerator>& generator) {
        messageGenerators[name].generators.push_back(generator);
    }

    /// Check if any generator is present for a given type
    bool HasGenerators(const std::string& name) const {
        return messageGenerators.count(name) > 0;
    }

    /// Get all schema generators
    const std::vector<ComRef<IGenerator>>& GetSchemaGenerators() const {
        return schemaGenerators;
    }

    /// Get all message generators
    const std::vector<ComRef<IGenerator>>& GetMessageGenerators(const std::string& name) const {
        return messageGenerators.at(name).generators;
    }

private:
    /// Message bucket
    struct GeneratorBucket {
        std::vector<ComRef<IGenerator>> generators;
    };

    /// All schema generators
    std::vector<ComRef<IGenerator>> schemaGenerators;

    /// Buckets for message generators
    std::map<std::string, GeneratorBucket> messageGenerators;
};
