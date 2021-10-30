#pragma once

/// Global unique message id
using MessageID = uint32_t;

/// Invalid id
static constexpr MessageID InvalidMessageID = ~0u;

/// Message schema type
enum class MessageSchemaType {
    None,

    /// Static schema, stride of each message is constant, single message type
    Static,

    /// Dynamic schema, stride of each message is variable, single message type
    Dynamic,

    /// Ordered schema, stride of each message is variable, multiple message types
    Ordered
};

/// Schema information
struct MessageSchema {
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
    struct Header {
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
    struct Header {
        MessageID id;
        uint64_t  byteSize;
    };

    static MessageSchema GetSchema() {
        MessageSchema schema{};
        schema.id = InvalidMessageID;
        schema.type = MessageSchemaType::Ordered;
        return schema;
    }
};
