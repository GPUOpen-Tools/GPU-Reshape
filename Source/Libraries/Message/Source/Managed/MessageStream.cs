using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Message.CLR
{
    public interface IMessageStream
    {
        MessageSchema GetSchema();

        unsafe byte* GetData();

        int GetCount();

        ulong GetSize();
    }

    public sealed class ReadOnlyMessageStream : IMessageStream
    {
        public unsafe byte* GetData()
        {
            return ptr;
        }

        public MessageSchema GetSchema()
        {
            return schema;
        }

        public int GetCount()
        {
            return count;
        }

        public ulong GetSize()
        {
            return size;
        }

        public MessageSchema schema;

        public unsafe byte* ptr;

        public ulong size;

        public int count;
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

    public struct OrderedMessageIterator
    {
        public unsafe byte* Data;
        public unsafe byte* End;

        public uint GetMessageID()
        {
            unsafe
            {
                return ((OrderedMessageSchema.Header*)Data)->id;
            }
        }

        public bool IsEOS()
        {
            unsafe
            {
                return Data >= End;
            }
        }

        public bool Good()
        {
            unsafe
            {
                return Data < End;
            }
        }

        public void Next()
        {
            unsafe
            {
                Data += ((OrderedMessageSchema.Header*)Data)->byteSize;
            }
        }

        public T Get<T>()
        {
            unsafe
            {
                return (T)Marshal.PtrToStructure((IntPtr)Data + Marshal.SizeOf(typeof(OrderedMessageSchema.Header)), typeof(T));
            }
        }
    }

    public class OrderedMessageView<S> where S : IMessageStream
    {
        public OrderedMessageView(S _storage)
        {
            Debug.Assert(storage.GetSchema().type == MessageSchemaType.Ordered, "Ordered view on unordered stream");
        
            storage = _storage;
        }

        public OrderedMessageIterator GetIterator()
        {
            unsafe
            {
                return new OrderedMessageIterator
                {
                    Data = storage.GetData(),
                    End = storage.GetData() + storage.GetSize(),
                };
            }
        }

        public S storage;
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
