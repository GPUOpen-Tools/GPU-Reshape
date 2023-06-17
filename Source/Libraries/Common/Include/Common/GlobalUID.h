// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
#include "Assert.h"

// Std
#include <string_view>
#include <string>
#include <cstdint>
#include <memory>

// System
#ifdef _WIN64
#include <guiddef.h>
#endif

struct GlobalUID {
    static constexpr uint32_t kSize = 16;

    /// Zero initializer
    GlobalUID() {
        std::memset(uuid, 0x0, sizeof(uuid));
    }

    /// Create a new UUID
    /// \return
    static GlobalUID New();

    ///
    /// \param view
    /// \return
    static GlobalUID FromString(const std::string_view& view);

    /// Convert to string
    /// \return
    std::string ToString() const;

#ifdef _WIN64
    /// Convert to the platform GUID
    /// \return
    GUID AsPlatformGUID() const;
#endif

    /// Check if this guid is valid
    /// \return
    bool IsValid() const {
        char set = 0x0;
        for (char byte : uuid) {
            set |= byte;
        }
        return set != 0;
    }

    /// Compare two UUIDs
    bool operator==(const GlobalUID& other) const {
        return std::memcmp(uuid, other.uuid, sizeof(uuid)) == 0;
    }

    /// Compare two UUIDs
    bool operator!=(const GlobalUID& other) const {
        return std::memcmp(uuid, other.uuid, sizeof(uuid)) != 0;
    }

private:
    /// Guid data
    char uuid[kSize];
};
