using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Message.CLR
{
    // Base message traits, usage is devirtualized
    public interface IMessage
    {
        // Memory view of reader
        ByteSpan Memory { set; }
    }

    // Base stream traits, usage is devirtualized
    public interface IMessageStream
    {
        // Get the schema of this stream
        MessageSchema GetSchema();

        // Get the data span of this stream
        ByteSpan GetSpan();

        // Get the number of messages
        int GetCount();
    }

    public sealed class ReadOnlyMessageStream : IMessageStream
    {
        public MessageSchema GetSchema()
        {
            return Schema;
        }

        public int GetCount()
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

        // Top schema type
        public MessageSchema Schema;

        // Data address
        public unsafe byte* Ptr;

        // Byte size
        public ulong Size;

        // Number of messages
        public int Count;
    }

    public class StaticMessageView<T, S> where T : IStaticMessageSchema where S : IMessageStream
    {
        public StaticMessageView(S _storage)
        {
            storage = _storage;
        }

        public S storage;
    }

    public class DynamicMessageView<T, S> where T : IDynamicMessageSchema where S : IMessageStream
    {
        public DynamicMessageView(S _storage)
        {
            storage = _storage;
        }

        public S storage;
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
        public OrderedMessageView(S storage)
        {
            Debug.Assert(storage.GetSchema().type == MessageSchemaType.Ordered, "Ordered view on unordered stream");

            _storage = storage;
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

        //
        private S _storage;
    }

    public class StaticMessageView<T> : StaticMessageView<T, ReadOnlyMessageStream> where T : IStaticMessageSchema
    {
        public StaticMessageView(ReadOnlyMessageStream _storage) : base(_storage)
        {

        }
    }

    public class DynamicMessageView<T> : DynamicMessageView<T, ReadOnlyMessageStream> where T : IDynamicMessageSchema
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
