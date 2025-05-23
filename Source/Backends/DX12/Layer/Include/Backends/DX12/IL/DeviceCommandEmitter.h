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
#include <Backend/IL/Emitters/IDeviceCommandEmitter.h>

class DeviceCommandEmitter final : public IL::IDeviceCommandEmitter {
public:
    DeviceCommandEmitter(
        IL::Emitter<>& invariantEmitter,
        IL::ID deviceCommandSignaturePtr,
        IL::ID deviceSourceCommandBufferPtr,
        IL::ID deviceDestCommandBufferPtr
    );

    /// Overrides
    IL::ID GetCount() override;
    IL::ID LoadType(IL::Emitter<>& emitter, IL::ID command) override;
    IL::ID LoadPayload(IL::Emitter<>& emitter, IL::ID command, uint32_t payloadIndex) override;
    void StorePayload(IL::Emitter<> &emitter, IL::ID command, uint32_t payloadIndex, IL::ID value) override;
    IL::DeviceCommandDispatchPayload LoadDispatchPayload(IL::Emitter<>& emitter, IL::ID command) override;

private:
    /// Load a signature dword
    /// @param emitter emitter
    /// @param offset dword offset
    /// @return id
    IL::ID LoadSignatureDWord(IL::Emitter<>& emitter, IL::ID offset);

    /// Load a command dword from source
    /// @param emitter emitter
    /// @param offset dword offset
    /// @return id
    IL::ID LoadSourceCommandDword(IL::Emitter<>& emitter, IL::ID offset);

    /// Invariant loads
    IL::ID count;
    IL::ID stride;

private:
    /// Source and destination ptrs
    IL::ID deviceCommandSignaturePtr;
    IL::ID deviceSourceCommandBufferPtr;
    IL::ID deviceDestCommandBufferPtr;
};

class DeviceCommandFormat : public IL::IDeviceCommandFormat {
public:
    COMPONENT(IDeviceCommandEmitter);

    /// Overrides
    virtual ComRef<IL::IDeviceCommandEmitter> CreateEmitter(
        IL::Emitter<>& invariantEmitter,
        IL::ID deviceCommandSignaturePtr,
        IL::ID deviceSourceCommandBufferPtr,
        IL::ID deviceDestCommandBufferPtr
    );
};
