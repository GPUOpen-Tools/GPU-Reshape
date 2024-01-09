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
