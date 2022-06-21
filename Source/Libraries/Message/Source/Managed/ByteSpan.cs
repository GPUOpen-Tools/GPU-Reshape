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
                return new ByteSpan(Pin, Data + offset, Length - offset);
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
