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

// Concurrency
#include <Features/Concurrency/ValidationListener.h>

// Message
#include <Message/MessageStream.h>

// Schema
#include <Schemas/Features/Concurrency.h>

ConcurrencyValidationListener::ConcurrencyValidationListener(ConcurrencyContainer &container): container(container) {
    
}

void ConcurrencyValidationListener::Handle(const MessageStream *streams, uint32_t count) {
    // Reinterpreted message data
    struct DebugMetadata {
        uint32_t token;
        uint32_t texelBaseOffsetAlign32;
        uint32_t texelOffset;
        uint32_t texelCountLiteral;
        uint32_t guardedTexelCount;
        uint32_t resourceTexelCount;
    };

    // For allocation reads
    std::lock_guard guard(container.mutex);

    // Handle all streams
    for (uint32_t i = 0; i < count; i++) {
        ConstMessageStreamView<ResourceRaceConditionMessage> view(streams[i]);

        // Validate all messages
        for (auto it = view.GetIterator(); it; ++it) {
            // Check primary key
            // TODO[init]: Add support for reading chunks in C++
            if (!(it->GetKey() & (1u << 31u))) {
                continue;
            }

            // Validate partial messages
            if (it.ptr + sizeof(uint32_t) + sizeof(DebugMetadata) > it.end) {
                break;
            }
            
            // Read beyond primary key
            auto* metadata = reinterpret_cast<const DebugMetadata*>(it.ptr + sizeof(uint32_t));

            // Read resource puid
            uint32_t puid = metadata->token & IL::kResourceTokenPUIDMask;

            // This isn't entirely CPU <-> GPU thread safe, for that we'd need to handle the resource versioning
            // but that's far too much for a little bit of validation.
            if (!container.allocations.contains(puid)) {
                continue;
            }

            ConcurrencyContainer::Allocation& alloc = container.allocations.at(puid);

            // Texel regions
            size_t texelEnd = metadata->texelOffset + metadata->guardedTexelCount;
            size_t blockEnd = (texelEnd + 31) / 32;

            // Validation
            ASSERT(metadata->texelBaseOffsetAlign32 >= alloc.memory.texelBaseBlock, "Base offset does not belong to resource");
            ASSERT(blockEnd <= alloc.memory.texelBlockCount, "Texel offset exceeds length");
        }
    }
}
