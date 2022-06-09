using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Message.CLR
{
    /// Represents an inline message indirection
    [StructLayout(LayoutKind.Explicit, Size = 8, CharSet = CharSet.Ansi)]
    public struct MessagePtr<T>
    {
        [FieldOffset(0)]
        public ulong thisOffset;

        public ref T Get()
        {
            throw new NotImplementedException();
            // return reinterpret_cast<T*>(reinterpret_cast<uint8_t>(this) + thisOffset);
        }
    };

    /// Represents an inline message array
    [StructLayout(LayoutKind.Explicit, Size = 16, CharSet = CharSet.Ansi)]
    public struct MessageArray<T>
    {
        [FieldOffset(0)]
        public ulong thisOffset;

        [FieldOffset(8)]
        public ulong count;

        public ref T[] Get()
        {
            throw new NotImplementedException();
            //return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + thisOffset);
        }

        public ref T this[ulong i]
        {
            get { return ref Get()[i]; }
        }
    };

    [StructLayout(LayoutKind.Explicit, Size = 16, CharSet = CharSet.Ansi)]
    public struct MessageString
    {
        bool Empty()
        {
            return data.count == 0;
        }

        void Set(string view)
        {
            Debug.Assert((ulong)view.Length <= data.count, "View length exceeds buffer size");
            throw new NotImplementedException();
        }

        string GetCopy()
        {
            return new string(data.Get());
        }

        [FieldOffset(0)]
        public MessageArray<char> data;
    };
}
