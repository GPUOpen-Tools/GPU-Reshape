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

namespace Message.CLR
{
    // Dynamic stream within inline stream
    public struct MessageSubStream
    {
        public ByteSpan Memory { set => _memory = value; }

        // Store a stream to this sub stream
        public void Store(IMessageStream stream)
        {
            if (stream.GetCount() == 0)
            {
                return;
            }

            // Write schema
            MessageSchema schema = stream.GetSchema();
            MemoryMarshal.Write<MessageSchemaType>(_memory.Slice(0, 4).AsRefSpan(), ref schema.type);
            MemoryMarshal.Write<uint>(_memory.Slice(4, 4).AsRefSpan(), ref schema.id);
            
            // Write count
            ulong count = stream.GetCount();
            MemoryMarshal.Write<ulong>(_memory.Slice(8, 8).AsRefSpan(), ref count);
            
            // Write data
            Data.Store(stream.GetSpan());
        }

        // Get the schema of this sub stream
        public MessageSchema Schema
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return new MessageSchema
                {
                    type = MemoryMarshal.Read<MessageSchemaType>(_memory.Slice(0, 4).AsRefSpan()),
                    id = MemoryMarshal.Read<uint>(_memory.Slice(4, 4).AsRefSpan())
                };
            }
        }

        // Get the number of messages in this sub stream
        public ulong Count
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get
            {
                return MemoryMarshal.Read<ulong>(_memory.Slice(8, 8).AsRefSpan());
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
