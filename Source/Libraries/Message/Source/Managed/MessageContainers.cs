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

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetThisOffset(ulong value)
        {
            MemoryMarshal.Write<ulong>(_memory.Slice(0, 8).AsRefSpan(), ref value);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetValue(T value)
        {
            int ByteWidth = Marshal.SizeOf<T>();
            MemoryMarshal.Write<T>(_memory.Slice((int)ThisOffset, ByteWidth).AsRefSpan(), ref value);
        }

        // Offset to current structure
        public ulong ThisOffset
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => MemoryMarshal.Read<ulong>(_memory.Slice(0, 8).AsRefSpan());

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => MemoryMarshal.Write<ulong>(_memory.Slice(0, 8).AsRefSpan(), ref value);
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

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                int ByteWidth = Marshal.SizeOf<T>();
                MemoryMarshal.Write<T>(_memory.Slice((int)ThisOffset, ByteWidth).AsRefSpan(), ref value);
            }
        }

        private ByteSpan _memory;
    };

    // Dynamic array in inline stream
    public struct MessageArray<T> where T : struct
    {
        public ByteSpan Memory { set => _memory = value; }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetThisOffset(ulong value)
        {
            MemoryMarshal.Write<ulong>(_memory.Slice(0, 8).AsRefSpan(), ref value);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetCount(int value)
        {
            MemoryMarshal.Write<int>(_memory.Slice(8, 16).AsRefSpan(), ref value);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void SetValue(int index, T value)
        {
            int ByteWidth = Marshal.SizeOf<T>();
            MemoryMarshal.Write<T>(_memory.Slice(ThisOffset + index * ByteWidth, ByteWidth).AsRefSpan(), ref value);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public void Store(ByteSpan span)
        {
            span.AsRefSpan().CopyTo(_memory.Slice(ThisOffset, span.Length).AsRefSpan());
        }

        // Offset to current structure
        public int ThisOffset
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => MemoryMarshal.Read<int>(_memory.Slice(0, 8).AsRefSpan());

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => MemoryMarshal.Write<int>(_memory.Slice(0, 8).AsRefSpan(), ref value);
        }

        // Number of elements in this array
        public int Count
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => MemoryMarshal.Read<int>(_memory.Slice(8, 16).AsRefSpan());

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => MemoryMarshal.Write<int>(_memory.Slice(8, 16).AsRefSpan(), ref value);
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

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set
            {
                int ByteWidth = Marshal.SizeOf<T>();
                MemoryMarshal.Write<T>(_memory.Slice(ThisOffset + index * ByteWidth, ByteWidth).AsRefSpan(), ref value);
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
        public ByteSpan Memory { set => Array.Memory = value; }

        // Get the length of this string
        public int Length
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => Array.Count;
        }

        // Create a string object, allocates
        public string String
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                unsafe
                {
                    var count = Array.Count;
                    return count > 0 ? Encoding.ASCII.GetString(Array.GetDataStart(), count) : string.Empty;
                }
            }

            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set => SetString(value);
        }

        public void SetString(string value)
        {
            unsafe
            {
                byte[] bytes = Encoding.ASCII.GetBytes(value);
                Debug.Assert(bytes.Length == Array.Count);
                    
                // Copy all bytes
                for (int i = 0; i < bytes.Length; i++)
                {
                    Array.GetDataStart()[i] = bytes[i];
                }
            }
        }

        public MessageArray<byte> Array;
    }
}
