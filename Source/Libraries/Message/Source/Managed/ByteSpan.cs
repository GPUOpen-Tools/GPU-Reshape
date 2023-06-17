// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Message.CLR
{
    /// Memory view for streams
    public struct ByteSpan
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe ByteSpan(byte* data, int length)
        {
            Pin = null;
            Data = data;
            Length = length;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe ByteSpan(GCHandle? pin, byte* data, int length)
        {
            Pin = pin;
            Data = data;
            Length = length;
        }

        // Slice this span from an offset
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public ByteSpan Slice(int offset)
        {
            unsafe
            {
                return new ByteSpan(Pin, Data + offset, Math.Max(0, Length - offset));
            }
        }

        // Slice this span from an offset and length
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public ByteSpan Slice(int offset, int length)
        {
            unsafe
            {
                return new ByteSpan(Pin, Data + offset, length);
            }
        }

        // To core span
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public Span<byte> AsRefSpan()
        {
            unsafe
            {
                return new Span<byte>(Data, Length);
            }
        }

        // Is this span empty?
        public bool IsEmpty { get => Length == 0; }

        // Optional pin
        public GCHandle? Pin;

        // Underlying data
        public unsafe byte* Data;

        // Span length
        public int Length;
    }
}
