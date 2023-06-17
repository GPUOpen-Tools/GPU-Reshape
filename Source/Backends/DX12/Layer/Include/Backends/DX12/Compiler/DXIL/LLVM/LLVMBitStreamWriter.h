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

// Layer
#include <Backends/DX12/Compiler/DXStream.h>

// Std
#include <cstdint>
#include <algorithm>

// Common
#include <Common/Assert.h>

/// Bit stream reader
struct LLVMBitStreamWriter {
    template<typename T>
    using ChunkType = std::conditional_t<(sizeof(T) > 4), uint64_t, uint32_t>;

    /// Position within the stream
    struct Position {
        /// Determinet the dword delta
        /// \param lhs left hand position
        /// \param rhs right hand position
        /// \return dword count
        static uint64_t DWord(const Position& lhs, const Position& rhs) {
            ASSERT(lhs.bitOffset % 32 == 0, "LHS bit offset not aligned");
            ASSERT(rhs.bitOffset % 32 == 0, "RHS bit offset not aligned");
            uint64_t a = lhs.offset * (sizeof(uint64_t) / sizeof(uint32_t)) + lhs.bitOffset / 32;
            uint64_t b = rhs.offset * (sizeof(uint64_t) / sizeof(uint32_t)) + rhs.bitOffset / 32;
            return b - a;
        }

        uint64_t offset;
        uint8_t bitOffset;
    };

    LLVMBitStreamWriter(DXStream& stream) : stream(stream) {
        ptr = stream.NextWord64();
    }

    /// Add the DXIL header
    ///   ! Must be run before all other operations
    void AddHeaderValidation() {
        constexpr uint32_t kMagic =
            ('B' << 0) |
            ('C' << 8) |
            (0x0 << 16) |
            (0xC << 20) |
            (0xE << 24) |
            (0xD << 28);

        // Write header
        Variable(kMagic, 32);
    }

    /// Write a fixed type
    /// \tparam T type to be used
    /// \param value value to be written
    /// \param fixedWidth bit width of this type
    template<typename T>
    Position Fixed(T value, uint8_t fixedWidth = sizeof(T) * 8) {
        ASSERT(fixedWidth <= 64, "Fixed width must be less or equal to 64 bits");
        return Variable<T>(value, fixedWidth);
    }

    /// Patch a fixed type at given position
    /// \tparam T type to be used
    /// \param value value to be written
    /// \param fixedWidth bit width of this type
    template<typename T>
    void FixedPatch(const Position& position, T value, uint8_t fixedWidth = sizeof(T) * 8) {
        ASSERT(fixedWidth <= 64, "Fixed width must be less or equal to 64 bits");
        return VariablePatch<T>(position, value, fixedWidth);
    }

    /// Consume a fixed enum type
    /// \tparam T type to be used, underlying type also used for expansion
    /// \param value value to be written
    /// \param fixedWidth bit width of this type
    template<typename T>
    Position FixedEnum(T value, uint8_t fixedWidth = sizeof(T) * 8) {
        ASSERT(fixedWidth <= 64, "Enum width must be less or equal to 64 bits");
        return Variable(static_cast<std::underlying_type_t<T>>(value), fixedWidth);
    }

    /// Write a variable width value
    ///   Given N bits, 0..N-1 contains accumulated chunk. Bit N-1 indicates next chunk.
    /// \tparam T type to be used
    /// \param value the value to be written
    /// \param bitWidth the bit width of each chunk
    template<typename T>
    Position VBR(T value, uint8_t bitWidth) {
        using TChunk = ChunkType<T>;

        Position anchor = Pos();

        uint8_t chunkBitOffset = 1 << (bitWidth - 1);

        TChunk mask = chunkBitOffset - 1;

        do {
            TChunk masked = value & mask;

            // Next chunk?
            if (value > masked) {
                masked |= chunkBitOffset;
            }

            Variable(masked, bitWidth);

            value >>= bitWidth - 1;
        } while (value);

        return anchor;
    }

    /// Encode a signed LLVM value
    static uint64_t EncodeSigned(int64_t value) {
        int64_t shl = value << 1;
        return (value < 0) ? ((-shl) | 0x1) : shl;
    }

    /// Align the stream to 32 bits
    void AlignDWord() {
        if (bitOffset % 32 == 0) {
            return;
        }

        // On dword boundary?
        if (bitOffset < 32) {
            bitOffset = 32;
        } else {
            ptr = stream.NextWord64();
            bitOffset = 0;
        }
    }

    /// Write a set of dwords
    ///  ! Must be aligned beforehand
    /// \param data dword data
    /// \param wordCount number of words
    Position WriteDWord(const uint8_t* data, uint32_t wordCount) {
        const auto* wordData = reinterpret_cast<const uint32_t*>(data);

        Position anchor = Pos();

        // Must be aligned
        ASSERT(bitOffset % 32 == 0, "Unaligned dword write");

        // Any to write?
        if (!wordCount) {
            return anchor;
        }

        // On dword boundary?
        if (bitOffset != 0) {
            // Write first word out
            *ptr |= static_cast<uint64_t>(wordData[0]) << 32u;
            ptr = stream.NextWord64();

            // Next!
            wordData++;
            wordCount--;
        }

        // Is the result going to be aligned?
        const bool isAligned = wordCount % 2 == 0;

        // Write all missing words
        stream.AppendData(wordData, wordCount * sizeof(uint32_t));

        // Set new state
        ptr = stream.GetMutableData<uint64_t>();
        bitOffset = isAligned ? 0 : 32;

        return anchor;
    }

    /// Write a char6 value
    /// \param ch ansi char
    Position Char6(char ch) {
        Position anchor = Pos();

        // Encode it
        uint8_t encoded;
        if (ch >= 'a' && ch <= 'z') {
            encoded = (ch - 'a') + 0;
        } else if (ch >= 'A' && ch <= 'Z') {
            encoded = (ch - 'A') + 26;
        } else if (ch >= '0' && ch <= '9') {
            encoded = (ch - '0') + 52;
        } else if (ch == '.') {
            encoded = 62;
        } else if (ch == '_') {
            encoded = 63;
        } else {
            ASSERT(false, "Invalid char6 encoding");
            encoded = 0;
        }

        // Write out encoded char
        Variable<uint8_t>(encoded, 6);

        return anchor;
    }

    /// Write a variable width data type
    /// \tparam T type to be used
    /// \param value value to be written
    /// \param count bit count
    template<typename T>
    Position Variable(const T& value, uint8_t count) {
        uint8_t available = 64u - bitOffset;

        Position anchor = Pos();

        if (count > available) {
            *ptr |= static_cast<uint64_t>(value) << bitOffset;
            ptr = stream.NextWord64();
            *ptr |= static_cast<uint64_t>(value) >> available;
            bitOffset = count - available;
        } else if (count == available) {
            *ptr |= static_cast<uint64_t>(value) << bitOffset;
            ptr = stream.NextWord64();
            bitOffset = 0;
        } else {
            *ptr |= static_cast<uint64_t>(value) << bitOffset;
            bitOffset += count;
        }

        return anchor;
    }

    /// Write a variable width data type at given position
    /// \tparam T type to be used
    /// \param value value to be written
    /// \param count bit count
    template<typename T>
    void VariablePatch(const Position& position, const T& value, uint8_t count) {
        uint8_t available = 64u - position.bitOffset;

        uint64_t* patchPtr = stream.GetMutableData<uint64_t>() + position.offset;

        if (count > available) {
            *patchPtr |= static_cast<uint64_t>(value) << position.bitOffset;
            patchPtr++;
            *patchPtr |= static_cast<uint64_t>(value) >> available;
        } else if (count == available) {
            *patchPtr |= static_cast<uint64_t>(value) << position.bitOffset;
        } else {
            *patchPtr |= static_cast<uint64_t>(value) << position.bitOffset;
        }
    }

    /// Close this writer
    void Close() {
        const uint8_t wordCount = 2u - (bitOffset + 31) / 32;
        stream.Resize(static_cast<uint32_t>(stream.GetByteSize() - sizeof(uint32_t) * wordCount));
    }

    /// Get the position of this writer
    Position Pos() const {
        return Position{
            .offset = static_cast<uint64_t>(ptr - stream.GetData<uint64_t>()),
            .bitOffset = bitOffset
        };
    }

private:
    /// Underlying stream
    DXStream& stream;

    /// Current word64
    uint64_t* ptr{nullptr};

    /// Current bit offset
    uint8_t bitOffset{0};
};
