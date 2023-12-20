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

    /// Set the new version
    /// \param value
    void SetVersionID(uint32_t value) {
        versionID = value;
    }

    /// Validate against a schema or set a new one
    /// \param value
    void ValidateOrSetSchema(const MessageSchema& value) {
#ifndef NDEBUG
        if (schema.type != MessageSchemaType::None) {
            ASSERT(schema == value, "Source schema incompatible with destination schema");
            return;
        }
#endif // NDEBUG

        schema = value;
    }

    /// Validate against a schema or set a new one
    /// \param value
    void Validate(const MessageSchema& value) const {
        if (schema.type != MessageSchemaType::None) {
            ASSERT(schema == value, "Source schema incompatible with destination schema");
            return;
        }
    }

    /// Set the data of this stream
    /// \param data the data pointer, byte size [dataSize]
    /// \param dataSize byte siz eof data
    /// \param messageCount the number of messages within this stream
    void SetData(const void* data, uint64_t dataSize, uint64_t messageCount) {
        buffer.resize(dataSize);
        std::memcpy(buffer.data(), data, dataSize);
        count = messageCount;
    }

    /// Resize this stream
    /// \param dataSize size of the stream
    /// \return stream start
    uint8_t* ResizeData(uint64_t dataSize) {
        buffer.resize(dataSize);
        return buffer.data();
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

    /// Clear this stream, does not change the schema
    void ClearWithSchemaInvalidate() {
        count = 0;
        schema = {};
        versionID = 0;
        buffer.clear();
    }

    /// Swap this stream with another, schema must match
    void Swap(MessageStream& other) {
        // Attempt to inherit the schema
        ValidateOrSetSchema(other.schema);

        std::swap(count, other.count);
        std::swap(versionID, other.versionID);
        buffer.swap(other.buffer);
    }

    /// Append another container
    template<typename T>
    void Append(const T& other) {
        // Skip empty
        if (other.IsEmpty()) {
            return;
        }
    
        // Attempt to inherit the schema
        ValidateOrSetSchema(other.GetSchema());

        // Destination offset
        const size_t offset = buffer.size();

        // Copy all data
        buffer.resize(offset + other.GetByteSize());
        std::memcpy(buffer.data() + offset, other.GetDataBegin(), other.GetByteSize());

        // Add countsf
        count += other.GetCount();
    }

    /// Erase a message byte range
    /// \param begin byte begin
    /// \param end byte end
    void Erase(size_t begin, size_t end) {
        buffer.erase(buffer.begin() + begin, buffer.begin() + end);
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

    /// Get the stream version
    [[nodiscard]]
    uint32_t GetVersionID() const {
        return versionID;
    }

    /// Get the number of messages within this stream
    [[nodiscard]]
    uint64_t GetCount() const {
        return count;
    }

    /// Check if this stream is empty
    [[nodiscard]]
    bool IsEmpty() const {
        return buffer.empty();
    }

private:
    /// Current schema
    MessageSchema schema;

    /// Number of messages in this stream
    uint64_t count{0};

    /// Version of this stream
    uint32_t versionID{0};

    /// The underlying memory
    std::vector<uint8_t> buffer;
};

/// Schema representation
template<typename T, typename STREAM>
struct MessageStreamSchema;

/// Static schema, see MessageSchemaType::Static
template<typename STREAM>
struct MessageStreamSchema<StaticMessageSchema, STREAM> {
    MessageStreamSchema(STREAM& stream) : stream(stream) {

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
        ConstIterator operator++(int) {
            ConstIterator self = *this;
            ++(*this);
            return self;
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
        auto allocation = stream.template Allocate<T, typename T::Schema>(sizeof(T));
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
    STREAM& GetStream() const {
        return stream;
    }

private:
    STREAM& stream;
};

/// Chunked schema, see MessageSchemaType::Chunked
template<typename STREAM>
struct MessageStreamSchema<ChunkedMessageSchema, STREAM> {
    MessageStreamSchema(STREAM& stream) : stream(stream) {

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
        ConstIterator operator++(int) {
            ConstIterator self = *this;
            ++(*this);
            return self;
        }

        /// Pre increment
        ConstIterator& operator++() {
            ptr = ptr + T::MessageSize(Get());
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
        auto allocation = stream.template Allocate<T, typename T::Schema>(sizeof(T));
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
    STREAM& GetStream() const {
        return stream;
    }

private:
    STREAM& stream;
};

/// Dynamic schema, see MessageSchemaType::Dynamic
template<typename STREAM>
struct MessageStreamSchema<DynamicMessageSchema, STREAM> {
    MessageStreamSchema(STREAM& stream) : stream(stream) {

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
        ConstIterator operator++(int) {
            ConstIterator self = *this;
            ++(*this);
            return self;
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
        auto allocation = stream.template Allocate<T, typename T::Schema>(byteSize);

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
    STREAM& GetStream() const {
        return stream;
    }

private:
    STREAM& stream;
};

/// Ordered schema, see MessageSchemaType::Ordered
template<typename STREAM>
struct MessageStreamSchema<OrderedMessageSchema, STREAM> {
    MessageStreamSchema(STREAM& stream) : stream(stream) {

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
            ASSERT(Is(T::kID), "Invalid cast");
            return reinterpret_cast<const T*>(ptr + sizeof(OrderedMessageSchema::Header));
        }

        /// Get the header
        [[nodiscard]]
        const OrderedMessageSchema::Header* GetHeader() const {
            return reinterpret_cast<const OrderedMessageSchema::Header*>(ptr);
        }

        /// Get the byte size of the current message
        size_t GetByteSize() const {
            return sizeof(OrderedMessageSchema::Header) + reinterpret_cast<const OrderedMessageSchema::Header*>(ptr)->byteSize;
        }

        /// Post increment
        ConstIterator operator++(int) {
            ConstIterator self = *this;
            ++(*this);
            return self;
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
        auto allocation = stream.template Allocate<T, OrderedMessageSchema>(byteSize);

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
    STREAM& GetStream() const {
        return stream;
    }

private:
    STREAM& stream;
};

/// Static and dynamic message stream views
///  fx.  MessageStreamView<FooMessage> view
/// , schema selection is deduced from the type.
template<typename T = void, typename STREAM = MessageStream>
struct MessageStreamView {
    using MessageSchema       = typename T::Schema;
    using MessageStreamSchema = MessageStreamSchema<MessageSchema, STREAM>;
    using ConstIterator       = typename MessageStreamSchema::template ConstIterator<T>;

    MessageStreamView(STREAM& stream) : schema(stream) {
        stream.ValidateOrSetSchema(MessageSchema::GetSchema(T::kID));
    }

    /// Add a message
    T* Add(const typename T::AllocationInfo& info = {}) {
        return schema.template Add<T>(info);
    }

    /// Get the iterator
    [[nodiscard]]
    ConstIterator GetIterator() const {
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
template<typename STREAM>
struct MessageStreamView<void, STREAM> {
    using MessageSchema       = OrderedMessageSchema;
    using MessageStreamSchema = MessageStreamSchema<MessageSchema, STREAM>;
    using ConstIterator       = typename MessageStreamSchema::ConstIterator;

    MessageStreamView(STREAM& stream) : schema(stream) {
        stream.ValidateOrSetSchema(MessageSchema::GetSchema());
    }

    /// Add a message
    template<typename T>
    T* Add(const typename T::AllocationInfo& info = {}) {
        return schema.template Add<T>(info);
    }

    /// Get the iterator
    [[nodiscard]]
    ConstIterator GetIterator() const {
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

/// Static and dynamic message stream views
///  fx.  MessageStreamView<FooMessage> view
/// , schema selection is deduced from the type.
template<typename T = void, typename STREAM = MessageStream>
struct ConstMessageStreamView {
    using MessageSchema       = typename T::Schema;
    using MessageStreamSchema = MessageStreamSchema<MessageSchema, const STREAM>;
    using ConstIterator       = typename MessageStreamSchema::template ConstIterator<T>;

    ConstMessageStreamView(const STREAM& stream) : schema(stream) {
        stream.Validate(MessageSchema::GetSchema(T::kID));
    }

    /// Get the iterator
    [[nodiscard]]
    ConstIterator GetIterator() const {
        return schema.template GetIterator<T>();
    }

    /// Get the message stream
    [[nodiscard]]
    MessageStream& GetStream() const {
        return schema.GetStream();
    }

    /// Get the number of messages
    [[nodiscard]]
    uint32_t GetCount() const {
        return static_cast<uint32_t>(schema.GetStream().GetCount());
    }

private:
    MessageStreamSchema schema;
};

/// Ordered message stream view
///  fx. MessageStreamView view
template<typename STREAM>
struct ConstMessageStreamView<void, STREAM> {
    using MessageSchema       = OrderedMessageSchema;
    using MessageStreamSchema = MessageStreamSchema<MessageSchema, const STREAM>;
    using ConstIterator       = typename MessageStreamSchema::ConstIterator;

    ConstMessageStreamView(const STREAM& stream) : schema(stream) {
        stream.Validate(MessageSchema::GetSchema());
    }

    /// Get the iterator
    [[nodiscard]]
    ConstIterator GetIterator() const {
        return schema.GetIterator();
    }

    /// Get the message stream
    [[nodiscard]]
    MessageStream& GetStream() const {
        return schema.GetStream();
    }

    /// Get the number of messages
    [[nodiscard]]
    uint32_t GetCount() const {
        return schema.GetStream().GetCount();
    }

private:
    MessageStreamSchema schema;
};
