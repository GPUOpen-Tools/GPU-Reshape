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
#include <Common/Allocators.h>

// Std
#include <mutex>
#include <map>

struct AGSState {
    /// Add a new state
    /// \param key owning key
    /// \param state tracked state
    /// \return state supplied
    static AGSState* Add(void *key, AGSState *state) {
        std::lock_guard lock(Mutex);
        Table[key] = state;
        return state;
    }

    /// Get an existing state
    /// \param key owning key
    /// \return state, nullptr if not found
    static AGSState *Get(void *key) {
        if (!key) {
            return nullptr;
        }

        std::lock_guard lock(Mutex);
        return Table.at(key);
    }

    /// Remove a state
    /// \param key owning key
    static void Remove(void *key) {
        if (!key) {
            return;
        }

        std::lock_guard lock(Mutex);
        Table.erase(key);
    }

    /// User supplied version
    uint32_t versionMajor{0};
    uint32_t versionMinor{0};
    uint32_t versionPatch{0};

    /// Allocators used during creation
    Allocators allocators;

private:
    /// AGS states are not wrapped to avoid detour pollution
    static std::mutex                 Mutex;
    static std::map<void*, AGSState*> Table;
};
