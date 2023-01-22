using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Message.CLR
{
    // Dynamic stream within inline stream
    public struct MessageSubStream
    {
        public ByteSpan Memory { set => _memory = value; }

        // Get the schema of this sub stream
        public MessageSchema Schema
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return new MessageSchema
                {
                    type = MemoryMarshal.Read<MessageSchemaType>(_memory.Slice(0, 4).AsRefSpan()),
                    id = MemoryMarshal.Read<uint>(_memory.Slice(4, 8).AsRefSpan())
                };
            }
        }

        // Get the number of messages in this sub stream
        public ulong Count
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return MemoryMarshal.Read<ulong>(_memory.Slice(8, 16).AsRefSpan());
            }
        }

        // Get the underlying data view
        public MessageArray<byte> Data
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return new MessageArray<byte> { Memory = _memory.Slice(16) };
            }
        }

        // Get the stream type
        public ReadOnlyMessageStream Stream
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                MessageArray<byte> array = Data;

                unsafe
                {
                    return new ReadOnlyMessageStream
                    {
                        Ptr = array.GetDataStart(),
                        Size = (ulong)array.Count,
                        Count = Count,
                        Schema = Schema
                    };
                }
            }
        }

        private ByteSpan _memory;
    };
}
