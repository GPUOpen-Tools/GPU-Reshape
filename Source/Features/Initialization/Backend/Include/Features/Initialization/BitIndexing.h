#pragma once

// Backend
#include <Backend/IL/Emitter.h>

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
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();

    // Add global offset
    IL::ID globalElement = emitter.Add(baseElementAlign32, blockOffset);

    // Bit or at given bit
    emitter.StoreBuffer(emitter.Load(buffer), globalElement, value);
}
