#pragma once

// Common
#include <Common/Assert.h>

// Std
#include <vector>

// Message
#include "Message.h"

/// Stream allocation type
template<typename T, typename SCHEMA>
struct MessageStreamAllocation {
    typename SCHEMA::Header* header;
    T*                       message;
};

/// Schema header traits
template<typename T>
struct MessageHeaderTraits {
    using Type = T;

    static constexpr size_t kSize = sizeof(T);
};

/// No schema header
template<>
struct MessageHeaderTraits<void> {
    using Type = void*;

    static constexpr size_t kSize = 0;
};

/// Base message stream, typeless
struct MessageStream {
    MessageStream(MessageSchema schema = {}) : schema(schema) {

    }

    /// Set the new schema
    /// \param value
    void SetSchema(const MessageSchema& value) {
        schema = value;
    }

    /// Validate against a schema or set a new one
    /// \param value
    void ValidateOrSetSchema(const MessageSchema& value) {
        if (schema.type != MessageSchemaType::None) {
            ASSERT(schema == value, "Source schema incompatible with destination schema");
            return;
        }

        schema = value;
    }

    /// Check if this stream hosts a given message
    template<typename T>
    bool Is() {
        using Schema = typename T::Schema;

        // Check the schema
        return Schema::GetSchema(T::kID) == schema;
    }

    /// Check if this stream hosts a given message, or is empty
    template<typename T>
    bool IsOrEmpty() {
        if (count == 0) {
            return true;
        }

        return Is<T>();
    }

    /// Allocate a new message
    template<typename T, typename SCHEMA>
    MessageStreamAllocation<T, SCHEMA> Allocate(uint64_t size) {
        using Traits = MessageHeaderTraits<typename SCHEMA::Header>;

        // Grow to new size
        size_t offset = buffer.size();
        buffer.resize(buffer.size() + size + Traits::kSize);

        // Set allocation pointers
        MessageStreamAllocation<T, SCHEMA> alloc;
        alloc.header  = reinterpret_cast<typename Traits::Type*>(&buffer[offset]);
        alloc.message = reinterpret_cast<T*>(&buffer[offset + Traits::kSize]);

        count++;
        return alloc;
    }

    /// Get the byte size of this stream
    [[nodiscard]]
    size_t GetByteSize() const {
        return buffer.size();
    }

    /// Clear this stream, does not change the schema
    void Clear() {
        count = 0;
        buffer.clear();
    }

    /// Swap this stream with another, schema must match
    void Swap(MessageStream& other) {
        // Attempt to inherit the schema
        ValidateOrSetSchema(other.schema);

        std::swap(count, other.count);
        buffer.swap(other.buffer);
    }

    /// Get the data begin pointer
    [[nodiscard]]
    const uint8_t* GetDataBegin() const {
        return buffer.data();
    }

    /// Get the data end pointer
    [[nodiscard]]
    const uint8_t* GetDataEnd() const {
        return buffer.data() + buffer.size();
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
        return count == 0;
    }

private:
    /// Current schema
    MessageSchema schema;

    /// Number of messages in this stream
    uint64_t count{0};

    /// The underlying memory
    std::vector<uint8_t> buffer;
};

/// Schema representation
template<typename T>
struct MessageStreamSchema;

/// Static schema, see MessageSchemaType::Static
template<>
struct MessageStreamSchema<StaticMessageSchema> {
    MessageStreamSchema(MessageStream& stream) : stream(stream) {

    }

    /// Static iterator
    template<typename T>
    struct ConstIterator {
        /// Get the message
        const T* Get() const {
            return reinterpret_cast<const T*>(ptr);
        }

        /// Accessor
        const T* operator->() const {
            return Get();
        }

        /// Post increment
        ConstIterator operator++(int) const {
            ConstIterator iterator;
            iterator.ptr = ptr + sizeof(T);
            iterator.end = end;
            return iterator;
        }

        /// Pre increment
        ConstIterator& operator++() {
            ptr = ptr + sizeof(T);
            return *this;
        }

        /// Valid?
        operator bool() const {
            return ptr < end;
        }

        const uint8_t* ptr;
        const uint8_t* end;
    };

    /// Add a new message
    template<typename T>
    T* Add(const typename T::AllocationInfo& info) {
        auto allocation = stream.Allocate<T, typename T::Schema>(sizeof(T));
        return new (allocation.message) T();
    }

    /// Get the message iterator for the stream
    template<typename T>
    ConstIterator<T> GetIterator() const {
        ConstIterator<T> iterator;
        iterator.ptr = stream.GetDataBegin();
        iterator.end = stream.GetDataEnd();
        return iterator;
    }

    /// Get the message stream
    [[nodiscard]]
    MessageStream& GetStream() const {
        return stream;
    }

private:
    MessageStream& stream;
};

/// Dynamic schema, see MessageSchemaType::Dynamic
template<>
struct MessageStreamSchema<DynamicMessageSchema> {
    MessageStreamSchema(MessageStream& stream) : stream(stream) {

    }

    /// Dynamic iterator
    template<typename T>
    struct ConstIterator {
        /// Get the message
        const T* Get() const {
            return reinterpret_cast<const T*>(ptr + sizeof(DynamicMessageSchema::Header));
        }

        /// Accessor
        const T* operator->() const {
            return Get();
        }

        /// Post increment
        ConstIterator operator++(int) const {
            ConstIterator iterator;
            iterator.ptr = ptr + sizeof(DynamicMessageSchema::Header) + reinterpret_cast<const DynamicMessageSchema::Header*>(ptr)->byteSize;
            iterator.end = end;
            return iterator;
        }

        /// Pre increment
        ConstIterator& operator++() {
            ptr += sizeof(DynamicMessageSchema::Header) + reinterpret_cast<const DynamicMessageSchema::Header*>(ptr)->byteSize;
            return *this;
        }

        /// Valid?
        operator bool() const {
            return ptr < end;
        }

        const uint8_t* ptr;
        const uint8_t* end;
    };

    /// Add a new message
    template<typename T>
    T* Add(const typename T::AllocationInfo& info) {
        uint64_t byteSize = info.ByteSize();

        // Allocate the message
        auto allocation = stream.Allocate<T, typename T::Schema>(byteSize);

        // Set the header
        allocation.header->byteSize = info.ByteSize();

        // Allocate and patch the inline data
        auto message = new (allocation.message) T();
        info.Patch(message);
        return message;
    }

    /// Get the dynamic iterator
    template<typename T>
    ConstIterator<T> GetIterator() const {
        ConstIterator<T> iterator;
        iterator.ptr = stream.GetDataBegin();
        iterator.end = stream.GetDataEnd();
        return iterator;
    }

    /// Get the message stream
    [[nodiscard]]
    MessageStream& GetStream() const {
        return stream;
    }

private:
    MessageStream& stream;
};

/// Ordered schema, see MessageSchemaType::Ordered
template<>
struct MessageStreamSchema<OrderedMessageSchema> {
    MessageStreamSchema(MessageStream& stream) : stream(stream) {

    }

    /// Ordered iterator
    struct ConstIterator {
        /// Get the id of the current message
        [[nodiscard]]
        MessageID GetID() const {
            return GetHeader()->id;
        }

        /// Check if the message is of type
        [[nodiscard]]
        bool Is(MessageID id) const {
            return GetID() == id;
        }

        /// Get the message
        template<typename T>
        const T* Get() const {
            return reinterpret_cast<const T*>(ptr + sizeof(OrderedMessageSchema::Header));
        }

        /// Get the header
        [[nodiscard]]
        const OrderedMessageSchema::Header* GetHeader() const {
            return reinterpret_cast<const OrderedMessageSchema::Header*>(ptr);
        }

        /// Post increment
        ConstIterator operator++(int) const {
            ConstIterator iterator;
            iterator.ptr = ptr + sizeof(OrderedMessageSchema::Header) + reinterpret_cast<const OrderedMessageSchema::Header*>(ptr)->byteSize;
            iterator.end = end;
            return iterator;
        }

        /// Pre increment
        ConstIterator& operator++() {
            ptr += sizeof(OrderedMessageSchema::Header) + reinterpret_cast<const OrderedMessageSchema::Header*>(ptr)->byteSize;
            return *this;
        }

        /// Valid?
        operator bool() const {
            return ptr < end;
        }

        const uint8_t* ptr;
        const uint8_t* end;
    };

    /// Add a new message
    template<typename T>
    T* Add(const typename T::AllocationInfo& info) {
        uint64_t byteSize = info.ByteSize();

        // Allocate the message
        auto allocation = stream.Allocate<T, OrderedMessageSchema>(byteSize);

        // Setup the header
        allocation.header->id       = T::kID;
        allocation.header->byteSize = info.ByteSize();

        // Patch any inline data
        auto message = new (allocation.message) T();
        info.Patch(message);
        return message;
    }

    /// Get the ordered iterator
    [[nodiscard]]
    ConstIterator GetIterator() const {
        ConstIterator iterator{};
        iterator.ptr = stream.GetDataBegin();
        iterator.end = stream.GetDataEnd();
        return iterator;
    }

    /// Get the message stream
    [[nodiscard]]
    MessageStream& GetStream() const {
        return stream;
    }

private:
    MessageStream& stream;
};

/// Static and dynamic message stream views
///  fx.  MessageStreamView<FooMessage> view
/// , schema selection is deduced from the type.
template<typename T = void>
struct MessageStreamView {
    using MessageSchema       = typename T::Schema;
    using MessageStreamSchema = MessageStreamSchema<MessageSchema>;

    MessageStreamView(MessageStream& stream) : schema(stream) {
        stream.ValidateOrSetSchema(MessageSchema::GetSchema(T::kID));
    }

    /// Add a message
    T* Add(const typename T::AllocationInfo& info = {}) {
        return schema.template Add<T>(info);
    }

    /// Get the iterator
    [[nodiscard]]
    typename MessageStreamSchema::template ConstIterator<T> GetIterator() const {
        return schema.template GetIterator<T>();
    }

    /// Get the message stream
    [[nodiscard]]
    MessageStream& GetStream() const {
        return schema.GetStream();
    }

private:
    MessageStreamSchema schema;
};

/// Ordered message stream view
///  fx. MessageStreamView view
template<>
struct MessageStreamView<void> {
    using MessageSchema       = OrderedMessageSchema;
    using MessageStreamSchema = MessageStreamSchema<MessageSchema>;

    MessageStreamView(MessageStream& stream) : schema(stream) {
        stream.ValidateOrSetSchema(MessageSchema::GetSchema());
    }

    /// Add a message
    template<typename T>
    T* Add(const typename T::AllocationInfo& info = {}) {
        return schema.template Add<T>(info);
    }

    /// Get the iterator
    [[nodiscard]]
    typename MessageStreamSchema::ConstIterator GetIterator() const {
        return schema.GetIterator();
    }

    /// Get the message stream
    [[nodiscard]]
    MessageStream& GetStream() const {
        return schema.GetStream();
    }

private:
    MessageStreamSchema schema;
};
