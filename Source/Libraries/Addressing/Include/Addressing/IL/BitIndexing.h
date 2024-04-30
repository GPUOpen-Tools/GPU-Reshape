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
#include <Backend/IL/Emitters/Emitter.h>

/// Perform an atomic or of a texel address
/// \param emitter instructione mitter
/// \param buffer destination buffer
/// \param baseElementAlign32 the base memory offset aligned to 32
/// \param texelOffset intra-resource texel offset
/// \param value given value to bit-or
/// \return existing value
template<typename T>
static IL::ID AtomicOrTexelAddress(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset, IL::ID value) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Extract element
    IL::ID element  = emitter.Div(texelOffset, constants.UInt(32)->id);

    // Add global offset
    IL::ID globalElement = emitter.Add(baseElementAlign32, element);

    // Bit or at given bit
    return emitter.AtomicOr(emitter.AddressOf(buffer, globalElement), value);
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
    
    return AtomicOrTexelAddress(emitter, buffer, baseElementAlign32, texelOffset, bit);
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

    // Extract element and global element
    IL::ID element       = emitter.Div(texelOffset, constants.UInt(32)->id);
    IL::ID globalElement = emitter.Add(baseElementAlign32, element);

    // Perform atomic and at address
    IL::ID value = emitter.AtomicAnd(emitter.AddressOf(buffer, globalElement), constants.UInt(~0u)->id);

    // Extract bit
    IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
    IL::ID bit      = emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);

    // Only report the texel bit itself, ignore the rest
    return emitter.BitAnd(value, bit);
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

    // Extract element and global element
    IL::ID element       = emitter.Div(texelOffset, constants.UInt(32)->id);
    IL::ID globalElement = emitter.Add(baseElementAlign32, element);

    // Perform atomic and at address
    IL::ID value = emitter.Extract(emitter.LoadBuffer(emitter.Load(buffer), globalElement), constants.UInt(0)->id);

    // Extract bit
    IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
    IL::ID bit      = emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);

    // Only report the texel bit itself, ignore the rest
    return emitter.BitAnd(value, bit);
}

/// Perform an atomic or of a texel address
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
