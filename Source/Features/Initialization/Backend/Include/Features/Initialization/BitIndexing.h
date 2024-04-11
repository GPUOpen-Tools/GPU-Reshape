#pragma once

#include <Backend/IL/Emitter.h>

template<typename T>
static IL::ID AtomicOrTexelAddress(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset, IL::ID value) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();
    
    IL::ID element  = emitter.Div(texelOffset, constants.UInt(32)->id);
    IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
    IL::ID bit      = emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);

    IL::ID globalElement = emitter.Add(baseElementAlign32, element);
    
    return emitter.AtomicOr(emitter.AddressOf(buffer, globalElement), value);
}

template<typename T>
static IL::ID AtomicOrTexelAddress(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();
    
    IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
    IL::ID bit      = emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);
    
    return AtomicOrTexelAddress(emitter, buffer, baseElementAlign32, texelOffset, bit);
}

template<typename T>
static IL::ID AtomicAndTexelAddress(IL::Emitter<T>& emitter, IL::ID buffer, IL::ID baseElementAlign32, IL::ID texelOffset) {
    Backend::IL::ConstantMap& constants = emitter.GetProgram()->GetConstants();
    
    IL::ID element       = emitter.Div(texelOffset, constants.UInt(32)->id);
    IL::ID globalElement = emitter.Add(baseElementAlign32, element);
    
    IL::ID value = emitter.AtomicAnd(emitter.AddressOf(buffer, globalElement), constants.UInt(~0u)->id);
    
    IL::ID bitIndex = emitter.Rem(texelOffset, constants.UInt(32)->id);
    IL::ID bit      = emitter.BitShiftLeft(constants.UInt(1u)->id, bitIndex);

    return emitter.BitAnd(value, bit);
}
