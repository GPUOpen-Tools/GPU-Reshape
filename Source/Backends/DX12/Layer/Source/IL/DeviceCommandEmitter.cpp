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

#include <Backends/DX12/IL/DeviceCommandEmitter.h>

DeviceCommandEmitter::DeviceCommandEmitter(
    IL::Emitter<>& invariantEmitter,
    IL::ID deviceCommandSignaturePtr,
    IL::ID deviceSourceCommandBufferPtr,
    IL::ID deviceDestCommandBufferPtr
) :
    deviceCommandSignaturePtr(deviceCommandSignaturePtr),
    deviceSourceCommandBufferPtr(deviceSourceCommandBufferPtr),
    deviceDestCommandBufferPtr(deviceDestCommandBufferPtr) {
    // Load invariants
    count = LoadSignatureDWord(invariantEmitter, invariantEmitter.UInt32(0));
    stride = LoadSignatureDWord(invariantEmitter, invariantEmitter.UInt32(1));
}

IL::ID DeviceCommandEmitter::GetCount() {
    return count;
}

IL::ID DeviceCommandEmitter::LoadType(IL::Emitter<>& emitter, IL::ID command) {
    return LoadSignatureDWord(emitter, emitter.Add(emitter.UInt32(2), command));
}

IL::ID DeviceCommandEmitter::LoadPayload(IL::Emitter<>& emitter, IL::ID command, IL::ID payloadIndex) {
    return LoadSourceCommandDword(emitter, emitter.Add(emitter.Mul(command, stride), emitter.UInt32(payloadIndex)));
}

void DeviceCommandEmitter::StorePayload(IL::Emitter<> &emitter, IL::ID command, uint32_t payloadIndex, IL::ID value) {
    emitter.StoreBuffer(
        emitter.Load(deviceDestCommandBufferPtr),
        emitter.Add(emitter.Mul(command, stride), emitter.UInt32(payloadIndex)),
        value
    );
}

IL::DeviceCommandDispatchPayload DeviceCommandEmitter::LoadDispatchPayload(IL::Emitter<>& emitter, IL::ID command) {
    return IL::DeviceCommandDispatchPayload {
        .groupCountX = LoadPayload(emitter, command, 0),
        .groupCountY = LoadPayload(emitter, command, 1),
        .groupCountZ = LoadPayload(emitter, command, 2)
    };
}

IL::ID DeviceCommandEmitter::LoadSignatureDWord(IL::Emitter<>& emitter, IL::ID offset) {
    return emitter.Extract(
        emitter.LoadBuffer(emitter.Load(deviceCommandSignaturePtr), offset),
        emitter.UInt32(0)
    );
}

IL::ID DeviceCommandEmitter::LoadSourceCommandDword(IL::Emitter<>& emitter, IL::ID offset) {
    return emitter.Extract(
        emitter.LoadBuffer(emitter.Load(deviceSourceCommandBufferPtr), offset),
        emitter.UInt32(0)
    );
}

ComRef<IL::IDeviceCommandEmitter> DeviceCommandFormat::CreateEmitter(
        IL::Emitter<>& invariantEmitter,
        IL::ID deviceCommandSignaturePtr,
        IL::ID deviceSourceCommandBufferPtr,
        IL::ID deviceDestCommandBufferPtr
) {
    return registry->New<DeviceCommandEmitter>(
        invariantEmitter,
        deviceCommandSignaturePtr,
        deviceSourceCommandBufferPtr,
        deviceDestCommandBufferPtr
    );
}
