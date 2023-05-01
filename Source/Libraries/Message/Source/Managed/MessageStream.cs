using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Message.CLR
{
    // Base message traits, usage is devirtualized
    public interface IMessage
    {
        // Memory view of reader
        ByteSpan Memory { set; }

        // Create a default allocation request
        IMessageAllocationRequest DefaultRequest();
    }

    // Message allocation, usage is devirtualized
    public interface IMessageAllocationRequest
    {
        // Patch a given message with the expected local structure from allocation parameters
        void Patch(IMessage message);

        // Get the expected byte size of the message
        ulong ByteSize { get; }

        // Get the message id
        uint ID { get; }
    }

    // Base stream traits, usage is devirtualized
    public interface IMessageStream
    {
        // Get the schema of this stream
        MessageSchema GetSchema();

        // Get the schema of this stream
        MessageSchema GetOrSetSchema(MessageSchema schema);

        // Get the data span of this stream
        ByteSpan GetSpan();

        // Allocate a new segment within this stream
        ByteSpan Allocate(int size);

        // Get the number of messages
        ulong GetCount();
    }

    public sealed class ReadOnlyMessageStream : IMessageStream
    {
        public MessageSchema GetSchema()
        {
            return Schema;
        }

        public MessageSchema GetOrSetSchema(MessageSchema schema)
        {
            if (Schema.type == MessageSchemaType.None)
            {
                Schema = schema;
            }
        
            return Schema;
        }

        public ulong GetCount()
        {
            return Count;
        }

        public ByteSpan GetSpan()
        {
            unsafe
            {
                return new ByteSpan(Ptr, (int)Size);
            }
        }

        public ByteSpan Allocate(int size)
        {
            throw new NotSupportedException("Allocation not supported on read only message streams");
        }

        // Optional pin
        public GCHandle? Pin;

        // Top schema type
        public MessageSchema Schema;

        // Data address
        public unsafe byte* Ptr;

        // Byte size
        public ulong Size;

        // Number of messages
        public ulong Count;
    }

    public sealed class ReadWriteMessageStream : IMessageStream
    {
        public MessageSchema GetSchema()
        {
            return Schema;
        }

        public MessageSchema GetOrSetSchema(MessageSchema schema)
        {
            if (Schema.type == MessageSchemaType.None)
            {
                Schema = schema;
            }

            return Schema;
        }

        public ulong GetCount()
        {
            return Count;
        }

        public ByteSpan GetSpan()
        {
            return GetSpan(0);
        }

        public ByteSpan Allocate(int size)
        {
            Count++;
            Data.SetLength(Data.Length + size);
            return GetSpan((int)Data.Length - size);
        }

        public ReadOnlyMessageStream ToReadOnly()
        {
            ByteSpan span = GetSpan(0);

            // Create stream
            unsafe
            {
                return new ReadOnlyMessageStream()
                {
                    Count = Count,
                    Pin = span.Pin,
                    Ptr = span.Data,
                    Schema = Schema,
                    Size = (ulong)span.Length
                };
            }
        }

        private ByteSpan GetSpan(int offset)
        {
            unsafe
            {
                byte[] byteData = Data.GetBuffer();

                // Pin the current data
                GCHandle pin = GCHandle.Alloc(byteData, GCHandleType.Pinned);

                // Construct span
                return new ByteSpan(pin, (byte*)pin.AddrOfPinnedObject() + offset, (int)Data.Length - offset);
            }
        }

        // Top schema type
        public MessageSchema Schema;

        // Number of messages
        public ulong Count;

        // Read write stream
        public MemoryStream Data = new MemoryStream();
    }

    public class StaticMessageView<T, S> : IEnumerable<T> where T : IMessage, new() where S : IMessageStream
    {
        public S Storage => _storage;

        public StaticMessageView(S storage)
        {
            // Create default request
            IMessageAllocationRequest request = new T().DefaultRequest();

            // Assign schema
            MessageSchema schema = storage.GetOrSetSchema(new MessageSchema { type = MessageSchemaType.Static, id = request.ID });
            
            // Validate
            Debug.Assert(
                schema.type == MessageSchemaType.Static,
                "Static view on non-static stream"
            );

            _storage = storage;
        }

        // Add a new message with given allocation request
        public T Add(IMessageAllocationRequest request)
        {
            // Allocate message
            T message = new T()
            {
                Memory = _storage.Allocate((int)request.ByteSize)
            };

            // Patch span
            request.Patch(message);

            // OK
            return message;
        }

        // Add a new message with default allocation requests
        public T Add() 
        {
            // Allocate message
            T message = new T();

            // Create default request
            IMessageAllocationRequest request = message.DefaultRequest();

            // Allocate message
            message.Memory = _storage.Allocate((int)request.ByteSize);

            // Patch span
            request.Patch(message);

            // OK
            return message;
        }

        // Enumerate this view
        public IEnumerator<T> GetEnumerator()
        {
            ByteSpan memory = _storage.GetSpan();

            // Create default request for iteration
            //   ? Acceptable for static message types
            IMessageAllocationRequest request = new T().DefaultRequest();

            // While messages to process
            while (!memory.IsEmpty)
            {
                // Create message
                yield return new T()
                {
                    Memory = memory
                };

                // Skip contents
                memory = memory.Slice((int)request.ByteSize);
            }
        }

        // Generic enumeration not supported
        IEnumerator IEnumerable.GetEnumerator()
        {
            throw new NotSupportedException();
        }

        private S _storage;
    }

    public class DynamicMessageView<T, S> : IEnumerable<T> where T : IMessage, new() where S : IMessageStream
    {
        public S Storage => _storage;

        public DynamicMessageView(S storage)
        {
            // Create default request
            IMessageAllocationRequest request = new T().DefaultRequest();

            // Assign schema
            MessageSchema schema = storage.GetOrSetSchema(new MessageSchema { type = MessageSchemaType.Dynamic, id = request.ID });
            
            // Validate
            Debug.Assert(
                schema.type == MessageSchemaType.Dynamic,
                "Dynamic view on non-dynamic stream"
            );

            _storage = storage;
        }

        // Add a new message with given allocation request
        public T Add(IMessageAllocationRequest request)
        {
            // Size of the schema header
            int headerSize = Marshal.SizeOf<DynamicMessageSchema.Header>();

            // Allocate header + message
            ByteSpan span = _storage.Allocate((int)request.ByteSize + headerSize);

            // Create schema
            var schema = new DynamicMessageSchema.Header
            {
                byteSize = request.ByteSize
            };

            // Write into message blob
            MemoryMarshal.Write(span.AsRefSpan(), ref schema);

            // Allocate message
            T message = new T()
            {
                Memory = span.Slice(headerSize)
            };

            // Patch span
            request.Patch(message);

            // OK
            return message;
        }

        // Add a new message with default allocation requests
        public T Add()
        {
            // Allocate message
            T message = new T();

            // Create default request
            IMessageAllocationRequest request = message.DefaultRequest();

            // Size of the schema header
            int headerSize = Marshal.SizeOf<DynamicMessageSchema.Header>();

            // Allocate header + message
            ByteSpan span = _storage.Allocate((int)request.ByteSize + headerSize);

            // Create schema
            var schema = new DynamicMessageSchema.Header
            {
                byteSize = request.ByteSize
            };

            // Write into message blob
            MemoryMarshal.Write(span.AsRefSpan(), ref schema);

            // Allocate message
            message.Memory = span.Slice(headerSize);

            // Patch span
            request.Patch(message);

            // OK
            return message;
        }

        // Enumerate this view
        public IEnumerator<T> GetEnumerator()
        {
            ByteSpan memory = _storage.GetSpan();

            // While messages to process
            while (!memory.IsEmpty)
            {
                // Get header
                var header = MemoryMarshal.Read<DynamicMessageSchema.Header>(memory.AsRefSpan());

                // Skip header to message contents
                memory = memory.Slice(Marshal.SizeOf<DynamicMessageSchema.Header>());

                // Create message
                yield return new T()
                {
                    Memory = memory
                };

                // Skip contents
                memory = memory.Slice((int)header.byteSize);
            }
        }

        // Generic enumeration not supported
        IEnumerator IEnumerable.GetEnumerator()
        {
            throw new NotSupportedException();
        }

        private S _storage;
    }

    // Ordered message enumerator type
    public struct OrderedMessage
    {
        public ByteSpan Memory { set => _memory = value; }

        // Assigned ID of message
        public uint ID;

        // Get a mesage of type
        public T Get<T>() where T : struct, IMessage
        {
            return new T() { Memory = _memory };
        }

        private ByteSpan _memory;
    }

    public class OrderedMessageView<S> : IEnumerable<OrderedMessage> where S : IMessageStream
    {
        public S Storage => _storage;

        public OrderedMessageView(S storage)
        {
            // Assign schema
            MessageSchema schema = storage.GetOrSetSchema(new MessageSchema { type = MessageSchemaType.Ordered, id = ~0u });
            
            // Validate
            Debug.Assert(
                schema.type == MessageSchemaType.Ordered,
                "Ordered view on unordered stream"
            );

            _storage = storage;
        }

        // Add a new message with given allocation request
        public T Add<T>(IMessageAllocationRequest request) where T : IMessage, new()
        {
            // Size of the schema header
            int headerSize = Marshal.SizeOf<OrderedMessageSchema.Header>();

            // Allocate header + message
            ByteSpan span = _storage.Allocate((int)request.ByteSize + headerSize);

            // Create schema
            var schema = new OrderedMessageSchema.Header
            {
                byteSize = request.ByteSize,
                id = request.ID
            };

            // Write into message blob
            MemoryMarshal.Write(span.AsRefSpan(), ref schema);

            // Allocate message
            T message = new T()
            {
                Memory = span.Slice(headerSize)
            };

            // Patch span
            request.Patch(message);

            // OK
            return message;
        }

        // Add a new message with default allocation requests
        public T Add<T>() where T : IMessage, new()
        {
            // Allocate message
            T message = new T();

            // Create default request
            IMessageAllocationRequest request = message.DefaultRequest();

            // Size of the schema header
            int headerSize = Marshal.SizeOf<OrderedMessageSchema.Header>();

            // Allocate header + message
            ByteSpan span = _storage.Allocate((int)request.ByteSize + headerSize);

            // Create schema
            var schema = new OrderedMessageSchema.Header
            {
                byteSize = request.ByteSize,
                id = request.ID
            };

            // Write into message blob
            MemoryMarshal.Write(span.AsRefSpan(), ref schema);

            // Allocate message
            message.Memory = span.Slice(headerSize);

            // Patch span
            request.Patch(message);

            // OK
            return message;
        }

        // Enumerate this view
        public IEnumerator<OrderedMessage> GetEnumerator()
        {
            ByteSpan memory = _storage.GetSpan();

            // While messages to process
            while (!memory.IsEmpty)
            {
                // Get header
                var header = MemoryMarshal.Read<OrderedMessageSchema.Header>(memory.AsRefSpan());

                // Skip header to message contents
                memory = memory.Slice(Marshal.SizeOf<OrderedMessageSchema.Header>());

                // Create message
                yield return new OrderedMessage()
                {
                    Memory = memory,
                    ID = header.id
                };

                // Skip contents
                memory = memory.Slice((int)header.byteSize);
            }
        }

        // Generic enumeration not supported
        IEnumerator IEnumerable.GetEnumerator()
        {
            throw new NotSupportedException();
        }

        private S _storage;
    }

    public class StaticMessageView<T> : StaticMessageView<T, ReadOnlyMessageStream> where T : IMessage, new()
    {
        public StaticMessageView(ReadOnlyMessageStream _storage) : base(_storage)
        {

        }
    }

    public class DynamicMessageView<T> : DynamicMessageView<T, ReadOnlyMessageStream> where T : IMessage, new()
    {
        public DynamicMessageView(ReadOnlyMessageStream _storage) : base(_storage)
        {

        }
    }

    public class OrderedMessageView : OrderedMessageView<ReadOnlyMessageStream>
    {
        public OrderedMessageView(ReadOnlyMessageStream _storage) : base(_storage)
        {

        }
    }
}
