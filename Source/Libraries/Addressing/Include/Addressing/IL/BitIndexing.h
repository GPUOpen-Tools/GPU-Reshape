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

// Backend
#include <Backend/IL/Emitters/ExtendedEmitter.h>
#include <Backend/IL/Emitters/Emitter.h>

/// Perform an atomic or of a texel address
/// \param emitter instructione mitter
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param texelOffset intra-resource texel offset
/// \param value given value to bit-or
/// \return existing value
template<typename T>
static IL::ID AtomicOrTexelAddressValue(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset, IL::ID value) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Extract element
    IL::ID element  = emitter.Div(texelOffset, constants.UInt(32)->id);

    // Add global offset
    IL::ID globalElement = emitter.Add(baseElementAlign32, element);

    // Bit or at given bit
    IL::ID atomicValue = emitter.AtomicOr(emitter.AddressOf(buffer, globalElement), value);
    
    // Only report the texel bit itself, ignore the rest
    return emitter.BitAnd(atomicValue, value);
}

/// Get the bit used for block-wise addressing
/// \param emitter instruction emitter
/// \param texelOffset non-block texel offset
/// \return bit
template<typename T>
static IL::ID GetTexelAddressBit(IL::Emitter<T>& emitter, IL::ID texelOffset) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Extract bit
    IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
    return emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);
}

/// Perform an atomic or of a texel address
/// \param emitter instructione mitter
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param texelOffset intra-resource texel offset
/// \return existing value
template<typename T>
static IL::ID AtomicOrTexelAddress(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Extract bit
    IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
    IL::ID bit      = emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);
    
    return AtomicOrTexelAddressValue(emitter, buffer, baseElementAlign32, texelOffset, bit);
}

/// Get the number of atomic blocks needed to store something
/// \param byteWidth max number of bytes stored
/// \return number of blocks
static uint32_t GetNumAtomicBlocks(uint32_t byteWidth) {
    if (byteWidth == 1u) {
        return 1u;
    }

    // Always one block, any bytes from 2 and above, incremental at each 32, may overlap into the next block
    return 1u + (byteWidth + 31u) / 32u;
}

/// Perform an atomic and of a texel address
/// \param emitter instructione mitter
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param texelOffset intra-resource texel offset
/// \return existing value
template<typename T>
static IL::ID AtomicAndTexelAddressValue(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset, IL::ID value) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Extract element and global element
    IL::ID element       = emitter.Div(texelOffset, constants.UInt(32)->id);
    IL::ID globalElement = emitter.Add(baseElementAlign32, element);

    // Perform atomic and at address
    IL::ID atomicValue = emitter.AtomicAnd(emitter.AddressOf(buffer, globalElement), constants.UInt(~0u)->id);

    // Only report the texel bit itself, ignore the rest
    return emitter.BitAnd(atomicValue, value);
}

/// Perform an atomic and of a texel address
/// \param emitter instructione mitter
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param texelOffset intra-resource texel offset
/// \return existing value
template<typename T>
static IL::ID AtomicAndTexelAddress(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Extract bit
    IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
    IL::ID bit      = emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);

    return AtomicAndTexelAddressValue(emitter, buffer, baseElementAlign32, texelOffset, bit);
}

/// Perform an atomic clear of a texel address
/// \param emitter instructione mitter
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param texelOffset intra-resource texel offset
/// \return existing value
template<typename T>
static IL::ID AtomicClearTexelAddressValue(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset, IL::ID value) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Extract element and global element
    IL::ID element       = emitter.Div(texelOffset, constants.UInt(32)->id);
    IL::ID globalElement = emitter.Add(baseElementAlign32, element);

    // Flip it
    IL::ID bitNot = emitter.Not(value);

    // Perform atomic clear at address
    IL::ID atomicValue = emitter.AtomicAnd(emitter.AddressOf(buffer, globalElement), bitNot);

    // Only report the texel bit itself, ignore the rest
    return emitter.BitAnd(atomicValue, bitNot);
}

/// Perform an atomic clear of a texel address
/// \param emitter instructione mitter
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param texelOffset intra-resource texel offset
/// \return existing value
template<typename T>
static IL::ID AtomicClearTexelAddress(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Extract the bit and flip it
    IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
    IL::ID bit      = emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);

    return AtomicClearTexelAddressValue(emitter, buffer, baseElementAlign32, texelOffset, bit);
}

/// Perform a read of a texel address
/// \param emitter instructione mitter
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param texelOffset intra-resource texel offset
/// \return existing value
template<typename T>
static IL::ID ReadTexelAddressValue(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset, IL::ID value) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Extract element and global element
    IL::ID element       = emitter.Div(texelOffset, constants.UInt(32)->id);
    IL::ID globalElement = emitter.Add(baseElementAlign32, element);

    // Perform atomic and at address
    IL::ID readValue = emitter.Extract(emitter.LoadBuffer(emitter.Load(buffer), globalElement), constants.UInt(0)->id);

    // Only report the texel bit itself, ignore the rest
    return emitter.BitAnd(readValue, value);
}

/// Perform a read of a texel address
/// \param emitter instructione mitter
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param texelOffset intra-resource texel offset
/// \return existing value
template<typename T>
static IL::ID ReadTexelAddress(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Extract bit
    IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
    IL::ID bit      = emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);

    return ReadTexelAddressValue(emitter, buffer, baseElementAlign32, texelOffset, bit);
}

/// Perform a non-atomic read of a texel address
/// \param emitter instructione mitter
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param blockOffset the offset for the specific block
/// \param value given value to bit-or
/// \return existing value
template<typename T>
static void WriteTexelAddressBlock(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID blockOffset, IL::ID value) {
    // Add global offset
    IL::ID globalElement = emitter.Add(baseElementAlign32, blockOffset);

    // Bit or at given bit
    emitter.StoreBuffer(emitter.Load(buffer), globalElement, value);
}

// todo[init]: move out

/// Ignored combiner
template<typename OP>
struct RegionCombinerIgnore {
    IL::ID Default(IL::Emitter<OP>& emitter) {
        return IL::InvalidID;
    }
    
    IL::ID Combine(IL::Emitter<OP>& emitter, IL::ID last, IL::ID value, IL::ID mask) {
        return IL::InvalidID;
    }
};

/// Combiner that bit-or's all fetched values
template<typename OP>
struct RegionCombinerBitOr {
    IL::ID Default(IL::Emitter<OP>& emitter) {
        return emitter.UInt32(0);
    }
    
    IL::ID Combine(IL::Emitter<OP>& emitter, IL::ID last, IL::ID value, IL::ID mask) {
        return emitter.BitOr(last, value);
    }
};

/// Combiner that checks all fetched values against their expected masks
template<typename OP>
struct RegionCombinerAnyNotEqual {
    IL::ID Default(IL::Emitter<OP>& emitter) {
        return emitter.Bool(false);
    }
    
    IL::ID Combine(IL::Emitter<OP>& emitter, IL::ID last, IL::ID value, IL::ID mask) {
        return emitter.Or(last, emitter.NotEqual(value, mask));
    }
};

/// Perform an atomic texel operation across an address region
/// \tparam COMBINER the value combiner to be used
/// \param emitter target emitter
/// \param functor the op function to use
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param texelOffset intra-resource texel offset
/// \param texelCountLiteral number of texels known at compile-time
/// \param texelCountRuntime number of texels known at runtime
/// \return combiner result
template<template<typename> typename COMBINER, typename OP, typename FUNCTOR>
static IL::ID AtomicOpTexelAddressRegion(IL::Emitter<OP>& emitter, FUNCTOR&& functor, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset, uint32_t texelCountLiteral, IL::ID texelCountRuntime) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Default combiner result
    IL::ID result = COMBINER<OP>{}.Default(emitter);

    // If there's no bytes at all, just ignore it
    if (texelCountLiteral == 0u) {
        return result;
    }

    // Fast path for single texel counts
    if (texelCountLiteral == 1u) {
        IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
        IL::ID bit      = emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);

        // While the static byte size is 1, the runtime size may be zero due to out of bounds mechanics
        bit = emitter.Select(
            emitter.Equal(texelCountRuntime, constants.UInt(0)->id),
            constants.UInt(0)->id,
            bit
        );

        // Pass through op and combiner
        IL::ID existingMask = functor(emitter, buffer, baseElementAlign32, texelOffset, bit);
        return COMBINER<OP>{}.Combine(emitter, result, existingMask, bit);
    }

    // Determine the number of blocks needed
    ASSERT(texelCountLiteral != 0u, "Invalid byte size");
    const uint32_t numBlocks = GetNumAtomicBlocks(texelCountLiteral);

    // Total number of bytes written so far
    IL::ID bytesWritten = emitter.UInt32(0u);

    // Useful constants
    IL::ID _0 = constants.UInt(0)->id;
    IL::ID _32 = constants.UInt(32)->id;
    IL::ID fullMask = constants.UInt(~0u)->id;

    // Unroll each block
    for (uint32_t i = 0; i < numBlocks; i++) {
        IL::ID bitIndex = emitter.Rem(texelOffset, _32);

        // byteSizeRuntime - bytesWritten
        IL::ID bytesRemaining = emitter.Sub(texelCountRuntime, bytesWritten);

        // bytesRemaining < 32 ? ~0u >> (32 - bytesRemaining) : ~0u
        IL::ID mask = emitter.Select(
            emitter.LessThan(bytesRemaining, _32),
            emitter.BitShiftRight(fullMask, emitter.Sub(_32, bytesRemaining)),
            fullMask
        );

        // mask << index
        mask = emitter.BitShiftLeft(mask, bitIndex);

        // bytesWritten >= byteSizeRuntime ? 0 : mask
        mask = emitter.Select(
            emitter.GreaterThanEqual(bytesWritten, texelCountRuntime),
            _0,
            mask
        );

        // Pass through op and combiner
        IL::ID existingMask = functor(emitter, buffer, baseElementAlign32, texelOffset, mask);
        result = COMBINER<OP>{}.Combine(emitter, result, existingMask, mask);

        // 32 - bitIndex
        IL::ID regionWidth = emitter.Sub(_32, bitIndex);

        // Next block range
        texelOffset = emitter.Add(texelOffset, regionWidth);
        bytesWritten = emitter.Add(bytesWritten, regionWidth);
    }

    // OK
    return result;
}
