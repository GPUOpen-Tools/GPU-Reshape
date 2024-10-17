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
#include <Common/Containers/LinearBlockAllocator.h>

// Std
#include <string_view>
#include <type_traits>

enum class VDFNodeType {
    Dictionary,
    String
};

struct VDFNode {
    /// Check if the node is of type
    template<typename T>
    bool Is() {
        if constexpr (std::is_same_v<T, VDFNode>) {
            return true;
        } else {
            return type == T::kType;
        }
    }

    /// Reinterpret this node
    template<typename T>
    T* As() {
        ASSERT(Is<T>(), "Invalid cast");
        return static_cast<T*>(this);
    }

    /// Cast this node, nullable
    template<typename T>
    T* Cast() {
        if (!Is<T>()) {
            return nullptr;
        }
        
        return static_cast<T*>(this);
    }

    /// Underlying type
    VDFNodeType type;
};

struct VDFString : public VDFNode {
    static constexpr VDFNodeType kType = VDFNodeType::String;

    /// Owned string
    std::string_view string;
};

struct VDFDictionaryEntry {
    /// Name of this entry
    std::string_view key;

    /// Value of this entry
    VDFNode* node;
};

struct VDFDictionaryNode : public VDFNode {
    static constexpr VDFNodeType kType = VDFNodeType::Dictionary;

    /// Find an entry with type
    /// \param view name to search for
    /// \return nullptr if not found
    template<typename T = VDFNode>
    T* Find(std::string_view view) {
        for (uint32_t i = 0; i < entryCount; i++) {
            if (entries[i].key == view) {
                return entries[i].node->Cast<T>();
            }
        }

        return nullptr;
    }

    /// Number of entries
    uint32_t entryCount;

    /// All entries
    VDFDictionaryEntry* entries;
};

/// Get the first root node
/// VDF's follow a similar format with a root node, this avoids common boilerplate
/// \param node root node
/// \return nullptr if not found
template<typename T>
static inline T* GetFirstVDFNode(VDFNode* node) {
    // Must be a single entry dictionary
    auto* dict = node->Cast<VDFDictionaryNode>();
    if (!dict || dict->entryCount != 1) {
        return nullptr;
    }

    // Try to cast
    return dict->entries[0].node->Cast<T>();
}

/// Shared allocator
using VDFArena = LinearBlockAllocator<2048>;
