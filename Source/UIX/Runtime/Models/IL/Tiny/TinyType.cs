using System.Runtime.InteropServices;

namespace Studio.Models.IL.Tiny;

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct IntType
{
    public byte bitWidth;
    public byte signedness;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct FPType
{
    public byte bitWidth;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct VectorType
{
    public ushort containedType;
    public byte dimension;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct MatrixType
{
    public ushort containedType;
    public byte rows;
    public byte columns;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct PointerType
{
    public ushort containedType;
    public byte addressSpace;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct ArrayType
{
    public ushort elementType;
    public uint count;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct TextureType
{
    public ushort sampledType;
    public byte dimension;
    public byte multisampled;
    public byte samplerMode;
    public byte format;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct BufferType
{
    public ushort sampledType;
    public byte samplerMode;
    public byte texelType;
    public byte byteAddressing;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct FunctionType
{
    public ushort returnType;
    public uint parameterCount;
}

[StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Ansi)]
public struct StructType
{
    public uint memberCount;
}
