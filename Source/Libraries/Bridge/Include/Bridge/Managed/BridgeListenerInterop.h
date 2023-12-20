// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

#include <Bridge/IBridgeListener.h>
#include <Message/MessageStream.h>

// Managed
#include <vcclr.h>

namespace Bridge::CLR {
    public class BridgeListenerInterop : public ::IBridgeListener {
    public:
        COMPONENT(BridgeListenerInterop);

        /// Constructor
        BridgeListenerInterop(const gcroot<Bridge::CLR::IBridgeListener^>& handle) : handle(handle) {

        }

        /// Overrides
        virtual void Handle(const ::MessageStream* streams, uint32_t count) override {
            // TODO: Batch submission
            for (uint32_t i = 0; i < count; i++) {
                const ::MessageStream& stream = streams[i];

                // Create clr object
                auto clrStream = gcnew Message::CLR::ReadOnlyMessageStream;
                clrStream->Ptr = const_cast<uint8_t*>(stream.GetDataBegin());
                clrStream->Size = stream.GetByteSize();
                clrStream->Count = stream.GetCount();
                clrStream->VersionID = stream.GetVersionID();
                clrStream->Schema.type = static_cast<Message::CLR::MessageSchemaType>(stream.GetSchema().type);
                clrStream->Schema.id = stream.GetSchema().id;
                handle->Handle(clrStream, 1);
            }
        }

        /// Pinned handle
        gcroot<Bridge::CLR::IBridgeListener^> handle;
    };
}
