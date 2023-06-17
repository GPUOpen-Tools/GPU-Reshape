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

// Std
#include <cstdint>
#include <algorithm>

// Common
#include <Common/Assert.h>

/// Bit stream reader
struct LLVMBitStreamReader {
    template<typename T>
    using ChunkType = std::conditional_t<(sizeof(T) > 4), uint64_t, uint32_t>;

    LLVMBitStreamReader(const void *ptr, uint32_t length) :
        start(reinterpret_cast<const uint64_t *>(ptr)),
        ptr(reinterpret_cast<const uint64_t *>(ptr)),
        end(reinterpret_cast<const uint64_t *>(ptr) + length) {
        /* */
    }

    /// Validate the header of this bit stream
    ///   ! Must be run before all other operations
    /// \return success state
    bool ValidateAndConsume() {
        constexpr uint32_t kMagic =
            ('B' << 0) |
            ('C' << 8) |
            (0x0 << 16) |
            (0xC << 20) |
            (0xE << 24) |
            (0xD << 28);

        // Validate header
        return Variable<uint32_t>(32) == kMagic;
    }

    /// Is this stream in an error state?
    /// \return error state
    bool IsError() const {
        return errorState;
    }

    /// Consume a fixed type
    /// \tparam T type to be used, also used for expansion
    /// \param fixedWidth bit width of this type
    /// \return read value
    template<typename T>
    T Fixed(uint8_t fixedWidth = sizeof(T) * 8) {
        ASSERT(fixedWidth <= 64, "Enum width must be less or equal to 64 bits");
        return Variable<T>(fixedWidth);
    }

    /// Consume a fixed enum type
    /// \tparam T type to be used, underlying type also used for expansion
    /// \param fixedWidth bit width of this type
    /// \return read value
    template<typename T>
    T FixedEnum(uint8_t fixedWidth = sizeof(T) * 8) {
        ASSERT(fixedWidth <= 64, "Enum width must be less or equal to 64 bits");
        return static_cast<T>(Variable<std::underlying_type_t<T>>(fixedWidth));
    }

    /// Consume a variable width value
    ///   Given N bits, 0..N-1 contains accumulated chunk. Bit N-1 indicates next chunk.
    /// \tparam T type to be used, underlying type also used for expansion
    /// \param bitWidth the bit width of each chunk
    /// \return accumulated value
    template<typename T>
    T VBR(uint8_t bitWidth) {
        using TChunk = ChunkType<T>;

        T value = 0;

        uint8_t chunkBitOffset = 1 << (bitWidth - 1);

        TChunk mask = chunkBitOffset - 1;

        for (TChunk shift = 0;; shift += (bitWidth - 1)) {
            auto chunk = Variable<TChunk>(bitWidth);

            value += (chunk & mask) << shift;

            // Next chunk?
            if (!(chunk & chunkBitOffset)) {
                break;
            }
        }

        return value;
    }

    /// Decode a signed LLVM value
    static int64_t DecodeSigned(uint64_t value) {
        uint64_t shr = value >> 1;
        return (value & 0x1) ? -static_cast<int64_t>(shr) : static_cast<int64_t>(shr);
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
            bitOffset = 0;
            ptr++;
        }
    }

    /// Read the char6 value
    /// \return read value
    char Char6() {
        auto encoded = Variable<uint8_t>(6);

        if (encoded <= 25) {
            return char('a' + encoded);
        } else if (encoded <= 51) {
            return char('A' + encoded - 26);
        } else if (encoded <= 61) {
            return char('0' + encoded - 52);
        } else if (encoded == 62) {
            return '.';
        } else if (encoded == 63) {
            return '_';
        }

        // Debugging
        ASSERT(false, "Invalid Char6 in LLVM bit-stream");

        // Mark as failed
        errorState = true;
        return 0x0;
    }

    /// Read a variable width data type
    /// \tparam T type to be used, underlying type also used for expansion
    /// \param count bit count
    /// \return read value
    template<typename T>
    T Variable(uint8_t count) {
        uint8_t available = 64u - bitOffset;

        T mask = static_cast<T>((1ull << count) - 1ull);

        T data;
        if (count > available) {
            data = static_cast<T>((*ptr >> bitOffset) | (*(ptr + 1) << available));
        } else {
            data = static_cast<T>(*ptr >> bitOffset);
        }

        bitOffset += count;

        ptr += bitOffset / 64;
        bitOffset = bitOffset % 64;

        return data & mask;
    }

    /// Get the safe data address for a given bit offset
    ///   ! Must be aligned to 8 bits
    /// \return the data address
    const uint8_t* GetSafeData() const {
        ASSERT(bitOffset % 8 == 0, "Unaligned data access, align beforehand");
        return reinterpret_cast<const uint8_t*>(ptr) + (bitOffset / 8);
    }

    /// Skip a number of bytes
    /// \param byteCount number of bytes to skip
    void Skip(uint32_t byteCount) {
        uint32_t bits = byteCount * 8;

        // Number of remaining bits
        uint8_t boundaryBits = 64u - bitOffset;

        // Within the boundary?
        if (bits <= boundaryBits) {
            bitOffset += bits;

            // On the boundary?
            if (bits == 64) {
                ptr++;
                bitOffset = 0;
            }
        } else {
            // Push boundary
            bits -= boundaryBits;

            // Push number of full 64 advances (+1 for the boundary)
            ptr += 1 + (bits / 64);

            // Push remaining
            bitOffset = bits % 64;
        }
    }

    /// Is this stream EOS?
    bool IsEOS() const {
        return ptr >= end;
    }

private:
    /// Data pointers
    const uint64_t *start;
    const uint64_t *ptr;
    const uint64_t *end;

    /// Current bit offset
    uint8_t bitOffset{0};

    /// Encountered an error?
    bool errorState{false};
};
