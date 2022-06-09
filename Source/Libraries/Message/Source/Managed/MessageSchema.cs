using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Message.CLR
{
    /// Message schema type
    public enum MessageSchemaType : UInt32
    {
        None,

        /// Static schema, stride of each message is constant, single message type
        Static,

        /// Dynamic schema, stride of each message is variable, single message type
        Dynamic,

        /// Ordered schema, stride of each message is variable, multiple message types
        Ordered
    };

    [StructLayout(LayoutKind.Explicit, Size = 8, CharSet = CharSet.Ansi)]
    public struct MessageSchema
    {
        [FieldOffset(0)]
        public MessageSchemaType type;

        [FieldOffset(4)]
        public int id;
    }

    public interface IStaticMessageSchema { };

    public interface IDynamicMessageSchema { };

    public struct StaticMessageSchema
    {
        [StructLayout(LayoutKind.Explicit, Size = 0, CharSet = CharSet.Ansi)]
        public struct Header
        {

        }
    };

    public struct DynamicMessageSchema
    {
        [StructLayout(LayoutKind.Explicit, Size = 8, CharSet = CharSet.Ansi)]
        public struct Header
        {
            [FieldOffset(0)]
            public ulong byteSize;
        }
    };

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
