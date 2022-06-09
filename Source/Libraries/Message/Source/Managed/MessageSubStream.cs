using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Message.CLR
{
    [StructLayout(LayoutKind.Explicit, Size = 32, CharSet = CharSet.Ansi)]
    public struct MessageSubStream
    {
        /// Current schema
        [FieldOffset(0)]
        public MessageSchema schema;

        /// Number of messages in this stream
        [FieldOffset(8)]
        public ulong count;

        /// The underlying memory
        [FieldOffset(16)]
        public MessageArray<char> data;
    };
}
