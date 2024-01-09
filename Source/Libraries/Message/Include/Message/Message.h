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

// Std
#include <cstdint>

// Common
#include <Common/Align.h>

/// Global unique message id
using MessageID = uint32_t;

/// Invalid id
static constexpr MessageID InvalidMessageID = ~0u;

/// Message schema type
enum class MessageSchemaType : uint32_t {
    None,

    /// Static schema, stride of each message is constant, single message type
    Static,

    /// Dynamic schema, stride of each message is variable, single message type
    Dynamic,

    /// Ordered schema, stride of each message is variable, multiple message types
    Ordered,

    /// Chunked schema, stride of each primary message is constant, single message type, each message may be extended by a set of variable chunks
    Chunked
};

// MSVC tight packing
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

/// Schema information
struct ALIGN_PACK MessageSchema {
    MessageSchemaType type = MessageSchemaType::None;
    MessageID         id   = InvalidMessageID;

    bool operator==(const MessageSchema& other) const {
        return type == other.type && id == other.id;
    }
};

/// Static schema, see MessageSchemaType::Static
struct StaticMessageSchema {
    using Header = void;

    static constexpr MessageSchema GetSchema(MessageID id) {
        MessageSchema schema{};
        schema.id = id;
        schema.type = MessageSchemaType::Static;
        return schema;
    }
};

/// Dynamic schema, see MessageSchemaType::Dynamic
struct DynamicMessageSchema {
    struct ALIGN_PACK Header {
        uint64_t byteSize;
    };

    static MessageSchema GetSchema(MessageID id) {
        MessageSchema schema{};
        schema.id = id;
        schema.type = MessageSchemaType::Dynamic;
        return schema;
    }
};

/// Ordered schema, see MessageSchemaType::Ordered
struct OrderedMessageSchema {
    struct ALIGN_PACK Header {
        MessageID id;
        uint64_t  byteSize;
    };

    static_assert(sizeof(Header) == 12, "Unexpected ordered schema size");

    static MessageSchema GetSchema() {
        MessageSchema schema{};
        schema.id = InvalidMessageID;
        schema.type = MessageSchemaType::Ordered;
        return schema;
    }
};

/// Chunked schema, see MessageSchemaType::Chunked
struct ChunkedMessageSchema {
    using Header = void;

    static MessageSchema GetSchema(MessageID id) {
        MessageSchema schema{};
        schema.id = id;
        schema.type = MessageSchemaType::Chunked;
        return schema;
    }
};

// MSVC tight packing
#ifdef _MSC_VER
#pragma pack(pop)
#endif
