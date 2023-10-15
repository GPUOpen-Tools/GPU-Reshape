// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

// Message
#include "MessageStream.h"
#include "MessageContainers.h"

// MSVC tight packing
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

struct ALIGN_PACK MessageSubStream {
    using View = MessageStreamView<void, MessageSubStream>;

    /// Set the stream data
    /// \param stream the stream to be copied
    void Set(const MessageStream& stream) {
        ASSERT(stream.GetByteSize() == data.count, "Message sub-stream has incorrect byte size");
        std::memcpy(data.Get(), stream.GetDataBegin(), stream.GetByteSize());

        // Copy properties
        schema = stream.GetSchema();
        count = stream.GetCount();
    }

    /// Transfer the sub stream data to a stream
    /// \param out
    void Transfer(MessageStream& out) const {
        out.SetSchema(schema);
        out.SetData(data.Get(), data.count, static_cast<uint32_t>(count));
    }

    /// Swap this stream with another, schema must match
    void Validate(const MessageSchema& value) const {
        if (schema.type != MessageSchemaType::None) {
            ASSERT(schema == value, "Source schema incompatible with destination schema");
        }
    }

    /// Swap this stream with another, schema must match
    void ValidateOrSetSchema(const MessageSchema& value) {
        if (schema.type != MessageSchemaType::None) {
            ASSERT(schema == value, "Source schema incompatible with destination schema");
            return;
        }

        schema = value;
    }

    /// Get the data begin pointer
    [[nodiscard]]
    const uint8_t* GetDataBegin() const {
        return data.Get();
    }

    /// Get the data end pointer
    [[nodiscard]]
    const uint8_t* GetDataEnd() const {
        return data.Get() + data.count;
    }

    /// Get the current schema
    [[nodiscard]]
    const MessageSchema& GetSchema() const {
        return schema;
    }

    /// Get the number of messages within this stream
    [[nodiscard]]
    uint64_t GetCount() const {
        return count;
    }

    /// Check if this stream is empty
    [[nodiscard]]
    bool IsEmpty() const {
        return data.count == 0;
    }

    /// Get the view for this container
    /// \return
    View GetView() {
        return View(*this);
    }

    /// Current schema
    MessageSchema schema;

    /// Number of messages in this stream
    uint64_t count{0};

    /// The underlying memory
    MessageArray<uint8_t> data;
};

static_assert(sizeof(MessageSubStream) == 32, "Malformed sub-stream size");

// MSVC tight packing
#ifdef _MSC_VER
#pragma pack(pop)
#endif
