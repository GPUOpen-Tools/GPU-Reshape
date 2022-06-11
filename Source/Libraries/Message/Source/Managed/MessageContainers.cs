using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace Message.CLR
{
    // Single indirection in inline stream
    public struct MessagePtr<T> where T : struct
    {
        public ByteSpan Memory { set => _memory = value; }

        // Offset to current structure
        public ulong ThisOffset
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => MemoryMarshal.Read<ulong>(_memory.Slice(0, 8).AsRefSpan());
        }

        // Get value of this indirection
        public T Value
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                int ByteWidth = Marshal.SizeOf<T>();
                return MemoryMarshal.Read<T>(_memory.Slice((int)ThisOffset, ByteWidth).AsRefSpan());
            }
        }

        private ByteSpan _memory;
    };

    // Dynamic array in inline stream
    public struct MessageArray<T> where T : struct
    {
        public ByteSpan Memory { set => _memory = value; }

        // Offset to current structure
        public int ThisOffset
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => MemoryMarshal.Read<int>(_memory.Slice(0, 8).AsRefSpan());
        }

        // Number of elements in this array
        public int Count
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => MemoryMarshal.Read<int>(_memory.Slice(8, 16).AsRefSpan());
        }

        // Get an element of this array
        public T this[int index]
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                int ByteWidth = Marshal.SizeOf<T>();
                return MemoryMarshal.Read<T>(_memory.Slice(ThisOffset + index * ByteWidth, ByteWidth).AsRefSpan());
            }
        }

        // Get the unsafe start to this array
        public unsafe byte* GetDataStart()
        {
            return _memory.Data + ThisOffset;
        }

        private ByteSpan _memory;
    }

    // String in inline stream
    public struct MessageString
    {
        public ByteSpan Memory { set => _array.Memory = value; }

        // Get the length of this string
        public int Length
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => _array.Count;
        }

        // Create a string object, allocates
        public string String
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                unsafe
                {
                    return Encoding.ASCII.GetString(_array.GetDataStart(), _array.Count);
                }
            }
        }

        private MessageArray<byte> _array;
    }
}
