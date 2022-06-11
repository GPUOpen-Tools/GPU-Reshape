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
        Ordered
    };

    // Message schema
    public struct MessageSchema
    {
        public MessageSchemaType type;

        public int id;
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
