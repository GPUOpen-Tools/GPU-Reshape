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

// Backend
#include <Backend/ShaderExport.h>

// Common
#include <Common/CRC.h>

// Std
#include <cstdint>

/// Invalid file UID
static constexpr uint16_t kInvalidShaderSourceFileUID = 0xFFFF;

struct ShaderSourceMapping {
    /// Constructor
    ShaderSourceMapping()
        : shaderGUID(0),
          fileUID(kInvalidShaderSourceFileUID),
          line(0),
          column(0),
          basicBlockId(0),
          instructionIndex(0) { }

    /// Default copy
    ShaderSourceMapping(const ShaderSourceMapping&) = default;

    /// Equality comparator
    bool operator==(const ShaderSourceMapping &rhs) const {
        return shaderGUID == rhs.shaderGUID &&
               fileUID == rhs.fileUID &&
               line == rhs.line &&
               column == rhs.column &&
               basicBlockId == rhs.basicBlockId &&
               instructionIndex == rhs.instructionIndex;
    }

    /// Inequality comparator
    bool operator!=(const ShaderSourceMapping &rhs) const {
        return shaderGUID != rhs.shaderGUID ||
               fileUID != rhs.fileUID ||
               line != rhs.line ||
               column != rhs.column ||
               basicBlockId != rhs.basicBlockId ||
               instructionIndex != rhs.instructionIndex;
    }

    /// The global shader UID
    uint64_t shaderGUID{0};

    /// The internal file UID
    uint64_t fileUID : 16;

    /// Line of the mapping
    uint64_t line : 32;

    /// Column of the mapping
    uint64_t column : 16;

    /// Index of the basic hosting block
    uint64_t basicBlockId : 32;

    /// Index of the hosting instruction
    uint64_t instructionIndex : 32;

    /// SGUID value
    ShaderSGUID sguid{InvalidShaderSGUID};

    /// Padding for hashing
    uint32_t padding{0};
};

/// Sanity check
static_assert(sizeof(ShaderSourceMapping) == 32, "Unexpected size");

/// Hash for source mappings
template <>
struct std::hash<ShaderSourceMapping> {
    std::size_t operator()(const ShaderSourceMapping& value) const {
        // Skip SGUID
        return BufferCRC32Short(&value, sizeof(value) - sizeof(uint64_t));
    }
};
