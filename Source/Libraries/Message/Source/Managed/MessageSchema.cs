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

using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Message.CLR
{
    // Message schema type
    public enum MessageSchemaType : UInt32
    {
        None,

        // Static schema, stride of each message is constant, single message type
        Static,

        // Dynamic schema, stride of each message is variable, single message type
        Dynamic,

        // Ordered schema, stride of each message is variable, multiple message types
        Ordered,

        // Chunked schema, stride of each primary message is constant, single message type, each message may be extended by a set of variable chunks
        Chunked
    };

    // Message schema
    public struct MessageSchema
    {
        // Check if this schema is static and of id
        public bool IsStatic(uint id)
        {
            return type == MessageSchemaType.Static && this.id == id;
        }

        // Check if this schema is dynamic and of id
        public bool IsDynamic(uint id)
        {
            return type == MessageSchemaType.Dynamic && this.id == id;
        }

        // Check if this schema is ordered
        public bool IsOrdered()
        {
            return type == MessageSchemaType.Ordered;
        }
        
        // Check if this schema is static and of id
        public bool IsChunked(uint id)
        {
            return type == MessageSchemaType.Chunked && this.id == id;
        }

        // Underlying type of schema
        public MessageSchemaType type;

        // Optional message id of schema
        public uint id;
    }

    // Static type constraint
    public interface IStaticMessageSchema { };

    // Dynamic type constraint
    public interface IDynamicMessageSchema { };

    // Static schema, see MessageSchemaType
    public struct StaticMessageSchema
    {
        [StructLayout(LayoutKind.Explicit, Size = 0, CharSet = CharSet.Ansi)]
        public struct Header
        {

        }
    };

    // Dynamic schema, see MessageSchemaType
    public struct DynamicMessageSchema
    {
        [StructLayout(LayoutKind.Explicit, Size = 8, CharSet = CharSet.Ansi)]
        public struct Header
        {
            [FieldOffset(0)]
            public ulong byteSize;
        }
    };

    // Ordered schema, see MessageSchemaType
    public struct OrderedMessageSchema
    {
        [StructLayout(LayoutKind.Explicit, Size = 12, CharSet = CharSet.Ansi)]
        public struct Header
        {
            [FieldOffset(0)]
            public uint id;

            [FieldOffset(4)]
            public ulong byteSize;
        }
    };
}
