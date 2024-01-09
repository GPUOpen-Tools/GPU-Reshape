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

// Layer
#include "Spv.h"

// Std
#include <vector>

struct SpvIdMap {
    /// Set the identifier bound
    /// \param value address of bound
    void SetBound(uint32_t* value) {
        allocatedBound = value;

        idLookup.resize(*value);
        for (uint32_t i = 0; i < *value; i++) {
            idLookup[i] = i;
        }
    }

    /// Allocate a new identifier
    /// \return
    SpvId Allocate() {
        SpvId id = (*allocatedBound)++;
        idLookup.emplace_back(id);
        return id;
    }

    /// Get an identifier
    SpvId Get(SpvId id) {
        return idLookup[id];
    }

    /// Set a relocation identifier
    /// \param id the relocation identifier
    /// \param value the value assigned
    void Set(SpvId id, SpvId value) {
        idLookup.at(id) = value;
    }

    /// Get the current bound
    uint32_t GetBound() const  {
        return *allocatedBound;
    }

private:
    /// Bound address
    uint32_t* allocatedBound{nullptr};

    /// All relocations
    std::vector<SpvId> idLookup;
};
