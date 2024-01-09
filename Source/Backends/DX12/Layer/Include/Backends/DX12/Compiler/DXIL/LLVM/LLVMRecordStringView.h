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
#include "LLVMRecord.h"

// Common
#include <Common/Assert.h>
#include <Common/CRC.h>

// Std
#include <string_view>

struct LLVMRecordStringView {
    LLVMRecordStringView() = default;

    /// Construct from record at offset
    LLVMRecordStringView(const LLVMRecord &record, uint32_t offset) :
        operands(record.ops + offset),
        operandCount(record.opCount - offset) {
        ASSERT(offset <= record.opCount, "Out of bounds record string view");

        // Hash contents
        ComputeHash();
    }

    /// Length of this string
    uint32_t Length() const {
        return operandCount;
    }

    /// Copy string to destination
    /// \param out must be able to contain the string
    void Copy(char* out) const {
        for (uint32_t i = 0; i < operandCount; i++) {
            out[i] = static_cast<char>(operands[i]);
        }
    }

    /// Copy string to destination
    /// \param out must be able to contain the string, with the null terminator
    void CopyTerminated(char* out) const {
        Copy(out);
        out[operandCount] = '\0';
    }

    /// Check for equality with rhs string
    bool operator==(const std::string_view& rhs) const {
        if (rhs.length() != operandCount) {
            return false;
        }

        for (uint32_t i = 0; i < operandCount; i++) {
            if (rhs[i] != operands[i]) {
                return false;
            }
        }

        return true;
    }

    /// Check for equality with rhs string
    bool operator!=(const std::string_view& rhs) const {
        return !(*this == rhs);
    }

    /// Check if this string starts with a sub string
    bool StartsWith(const std::string_view& str) const {
        if (str.length() > operandCount) {
            return false;
        }

        for (uint32_t i = 0; i < str.length(); i++) {
            if (str[i] != operands[i]) {
                return false;
            }
        }

        return true;
    }

    /// Check if this string at a given offset starts with
    bool StartsWithOffset(uint64_t offset, const std::string_view& str) const {
        if (str.length() > operandCount - offset) {
            return false;
        }

        for (uint32_t i = 0; i < str.length(); i++) {
            if (str[i] != static_cast<char>(operands[offset + i])) {
                return false;
            }
        }

        return true;
    }

    /// Substr this view
    void SubStr(uint64_t begin, uint64_t end, char* buffer) const {
        for (size_t i = begin; i < end; i++) {
            buffer[i - begin] = static_cast<char>(operands[i]);
        }
    }

    /// Substr this view with null termination
    void SubStrTerminated(uint64_t begin, uint64_t end, char* buffer) const {
        for (size_t i = begin; i < end; i++) {
            buffer[i - begin] = static_cast<char>(operands[i]);
        }

        // Terminator
        buffer[end - begin] = '\0';
    }

    /// Copy until a given condition is met
    template<typename F>
    void CopyUntilTerminated(uint64_t begin, char* buffer, uint32_t length, F&& functor) {
        size_t i;
        for (i = begin; i < std::min<size_t>(begin + length - 1, operandCount); i++) {
            char ch = static_cast<char>(operands[i]);

            // Break?
            if (!functor(ch)) {
                break;
            }

            // Append
            buffer[i - begin] = ch;
        }

        // Terminator
        buffer[i - begin] = '\0';
    }

    /// Accessor
    char operator[](uint32_t i) const {
        ASSERT(i < operandCount, "Out of bounds index");
        return static_cast<char>(operands[i]);
    }

    /// Valid?
    operator bool() const {
        return operands != nullptr;
    }

    /// Get the precomputed hash
    uint64_t GetHash() const {
        return hash;
    }

private:
    void ComputeHash() {
        hash = ~0u;

        // Compute CRC
        for (uint32_t i = 0; i < operandCount; i++) {
            hash = kCRC32Table[static_cast<unsigned char>(operands[i]) ^ (hash & 0xFFu)] ^ (hash >> 8u);
        }

        hash = ~hash;
    }

private:
    /// Starting operand
    const uint64_t *operands{nullptr};

    /// Number of operands
    uint32_t operandCount{0};

    /// Precomputed hash
    uint32_t hash{0};
};
