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

#include <Bridge/Network/PingPongListener.h>
#include <Bridge/IBridge.h>

// Message
#include <Message/MessageStream.h>
#include <Message/IMessageStorage.h>

// Schemas
#include <Schemas/PingPong.h>

PingPongListener::PingPongListener(IBridge *owner) : bridge(owner) {

}

void PingPongListener::Handle(const MessageStream *streams, uint32_t count) {
    MessageStream outgoing;
    MessageStreamView<PingPongMessage> outgoingView(outgoing);

    // Read incoming pings
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView<PingPongMessage> view(streams[i]);

        // Pong all messages
        for (auto it = view.GetIterator(); it; ++it) {
            PingPongMessage* out = outgoingView.Add();
            out->timeStamp = it->timeStamp;
        }
    }

    // Add outgoing pongs
    bridge->GetOutput()->AddStream(outgoing);
}
