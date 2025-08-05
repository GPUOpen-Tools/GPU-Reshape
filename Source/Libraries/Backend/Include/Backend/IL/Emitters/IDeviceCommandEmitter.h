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
#include <Backend/IL/DeviceCommand.h>
#include <Backend/IL/Emitters/Emitter.h>

namespace IL {
    class IDeviceCommandEmitter : public IComponent {
    public:
        /// Get the number of commands
        virtual ID GetCount() = 0;

        /// Load the type of a sub-command
        /// @param emitter target emitter
        /// @param command command index
        /// @return id
        virtual ID LoadType(Emitter<>& emitter, ID command) = 0;

        /// Load the payload of a sub-command
        /// @param emitter target emitter
        /// @param command command index
        /// @param payloadIndex the dword payload index
        /// @return id
        virtual ID LoadPayload(Emitter<>& emitter, ID command, uint32_t payloadIndex) = 0;

        /// Store a payload of a sub-command
        /// @param emitter target emitter
        /// @param command command index
        /// @param payloadIndex the dword payload index
        /// @param value the dword to be stored
        virtual void StorePayload(Emitter<>& emitter, ID command, uint32_t payloadIndex, ID value) = 0;

        /// Load a dispatch payload, must be dispatch
        /// @param emitter target emitter
        /// @param command command index
        /// @return payload
        virtual DeviceCommandDispatchPayload LoadDispatchPayload(Emitter<>& emitter, ID command) = 0;
    };
    
    class IDeviceCommandFormat : public TComponent<IDeviceCommandFormat> {
    public:
        COMPONENT(IDeviceCommandEmitter);

        /// Create a new emitter
        /// @param invariantEmitter the shared emitter, ideally hosted at a header block
        /// @param deviceCommandSignaturePtr the device provided signature
        /// @param deviceSourceCommandBufferPtr the source commands
        /// @param deviceDestCommandBufferPtr the destination commands
        /// @return emitter
        virtual ComRef<IDeviceCommandEmitter> CreateEmitter(
            Emitter<>& invariantEmitter,
            ID deviceCommandSignaturePtr,
            ID deviceSourceCommandBufferPtr,
            ID deviceDestCommandBufferPtr
        ) = 0;
    };
}
