using System.Runtime.InteropServices;

namespace Studio.Models.IL.Tiny;

[StructLayout(LayoutKind.Explicit, Size = 2, CharSet = CharSet.Ansi)]
public struct IntType
{
    [FieldOffset(0)]
    public byte bitWidth;
    
    [FieldOffset(1)]
    public byte signedness;
}

[StructLayout(LayoutKind.Explicit, Size = 1, CharSet = CharSet.Ansi)]
public struct FPType
{
    [FieldOffset(0)]
    public byte bitWidth;
}

[StructLayout(LayoutKind.Explicit, Size = 3, CharSet = CharSet.Ansi)]
public struct VectorType
{
    [FieldOffset(0)]
    public ushort containedType;
    
    [FieldOffset(2)]
    public byte dimension;
}

[StructLayout(LayoutKind.Explicit, Size = 4, CharSet = CharSet.Ansi)]
public struct MatrixType
{
    [FieldOffset(0)]
    public ushort containedType;
    
    [FieldOffset(2)]
    public byte rows;

    [FieldOffset(3)]
    public byte columns;
}

[StructLayout(LayoutKind.Explicit, Size = 3, CharSet = CharSet.Ansi)]
public struct PointerType
{
    [FieldOffset(0)]
    public ushort containedType;
    
    [FieldOffset(2)]
    public byte addressSpace;
}

[StructLayout(LayoutKind.Explicit, Size = 6, CharSet = CharSet.Ansi)]
public struct ArrayType
{
    [FieldOffset(0)]
    public ushort elementType;
    
    [FieldOffset(2)]
    public uint count;
}

[StructLayout(LayoutKind.Explicit, Size = 6, CharSet = CharSet.Ansi)]
public struct TextureType
{
    [FieldOffset(0)]
    public ushort sampledType;
    
    [FieldOffset(2)]
    public byte dimension;
    
    [FieldOffset(3)]
    public byte multisampled;
    
    [FieldOffset(4)]
    public byte samplerMode;
    
    [FieldOffset(5)]
    public byte format;
}

[StructLayout(LayoutKind.Explicit, Size = 5, CharSet = CharSet.Ansi)]
public struct BufferType
{
    [FieldOffset(0)]
    public ushort sampledType;
    
    [FieldOffset(2)]
    public byte samplerMode;
    
    [FieldOffset(3)]
    public byte tex3elType;
    
    [FieldOffset(4)]
    public byte byteAddressing;
}

[StructLayout(LayoutKind.Explicit, Size = 6, CharSet = CharSet.Ansi)]
public struct FunctionType
{
    [FieldOffset(0)]
    public ushort returnType;
    
    [FieldOffset(2)]
    public uint parameterCount;
}

[StructLayout(LayoutKind.Explicit, Size = 4, CharSet = CharSet.Ansi)]
public struct StructType
{
    [FieldOffset(0)]
    public uint memberCount;
}
