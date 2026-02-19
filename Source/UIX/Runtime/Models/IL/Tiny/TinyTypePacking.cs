using System;
using System.Runtime.InteropServices;
using Message.CLR;

namespace Studio.Models.IL.Tiny;

public static class TinyTypePacking
{
    public class TinyTypeResolver
    {
        public ushort TypeID = 0;
    }
    
    /// <summary>
    /// Unpack a tiny type
    /// </summary>
    public static Type UnpackTinyType(ref ReadOnlySpan<byte> span, TinyTypeResolver resolver)
    {
        // Self
        uint id = resolver.TypeID++;

        // Read kind
        TypeKind kind = (TypeKind)MemoryMarshal.Read<byte>(span);
        span = span.Slice(sizeof(byte));

        Type opaqueType;
        switch (kind)
        {
            default:
            {
                throw new NotImplementedException($"Type {kind} is not implemented");
            }
            case TypeKind.Unexposed:
            {
                opaqueType = new IL.UnexposedType();
                break;
            }
            case TypeKind.Bool:
            {
                opaqueType = new IL.BoolType()
                {
                    Kind = kind,
                    ID = id
                };
                break;
            }
            case TypeKind.Void:
            {
                opaqueType = new IL.VoidType()
                {
                    Kind = kind,
                    ID = id
                };
                break;
            }
            case TypeKind.Int:
            {
                IntType header = MemoryMarshal.Read<IntType>(span);
                span = span.Slice(Marshal.SizeOf(typeof(IntType)));
                
                IL.IntType type = new()
                {
                    Kind = kind,
                    ID = id,
                    BitWidth = header.bitWidth,
                    Signedness = header.signedness == 1
                };

                opaqueType = type;
                break;
            }
            case TypeKind.FP:
            {
                FPType header = MemoryMarshal.Read<FPType>(span);
                span = span.Slice(Marshal.SizeOf(typeof(FPType)));
                
                IL.FPType type = new()
                {
                    Kind = kind,
                    ID = id,
                    BitWidth = header.bitWidth
                };

                opaqueType = type;
                break;
            }
            case TypeKind.Vector:
            {
                VectorType header = MemoryMarshal.Read<VectorType>(span);
                span = span.Slice(Marshal.SizeOf(typeof(VectorType)));
                
                IL.VectorType type = new()
                {
                    Kind = kind,
                    ID = id,
                    Dimension = header.dimension,
                    ContainedType = UnpackTinyType(ref span, resolver)
                };

                opaqueType = type;
                break;
            }
            case TypeKind.Matrix:
            {
                MatrixType header = MemoryMarshal.Read<MatrixType>(span);
                span = span.Slice(Marshal.SizeOf(typeof(MatrixType)));
                
                IL.MatrixType type = new()
                {
                    Kind = kind,
                    ID = id,
                    Rows = header.rows,
                    Columns = header.columns,
                    ContainedType = UnpackTinyType(ref span, resolver)
                };

                opaqueType = type;
                break;
            }
            case TypeKind.Pointer:
            {
                PointerType header = MemoryMarshal.Read<PointerType>(span);
                span = span.Slice(Marshal.SizeOf(typeof(PointerType)));
                
                IL.PointerType type = new()
                {
                    Kind = kind,
                    ID = id,
                    AddressSpace = (AddressSpace)header.addressSpace,
                    Pointee = UnpackTinyType(ref span, resolver)
                };

                opaqueType = type;
                break;
            }
            case TypeKind.Array:
            {
                ArrayType header = MemoryMarshal.Read<ArrayType>(span);
                span = span.Slice(Marshal.SizeOf(typeof(ArrayType)));
                
                IL.ArrayType type = new()
                {
                    Kind = kind,
                    ID = id,
                    Count = header.count,
                    ElementType = UnpackTinyType(ref span, resolver)
                };

                opaqueType = type;
                break;
            }
            case TypeKind.Texture:
            {
                TextureType header = MemoryMarshal.Read<TextureType>(span);
                span = span.Slice(Marshal.SizeOf(typeof(TextureType)));
                
                IL.TextureType type = new()
                {
                    Kind = kind,
                    ID = id,
                    Dimension = (TextureDimension)header.dimension,
                    Format = (Format)header.format,
                    Multisampled = header.multisampled == 1,
                    SamplerMode = (ResourceSamplerMode)header.samplerMode,
                    SampledType = UnpackTinyType(ref span, resolver)
                };

                opaqueType = type;
                break;
            }
            case TypeKind.Buffer:
            {
                BufferType header = MemoryMarshal.Read<BufferType>(span);
                span = span.Slice(Marshal.SizeOf(typeof(BufferType)));
                
                IL.BufferType type = new()
                {
                    Kind = kind,
                    ID = id,
                    SamplerMode = (ResourceSamplerMode)header.samplerMode,
                    TexelType = (Format)header.texelType,
                    ElementType = UnpackTinyType(ref span, resolver)
                };

                opaqueType = type;
                break;
            }
            case TypeKind.Sampler:
            {
                opaqueType = new IL.SamplerType()
                {
                    Kind = kind,
                    ID = id
                };
                break;
            }
            case TypeKind.CBuffer:
            {
                opaqueType = new IL.CBufferType()
                {
                    Kind = kind,
                    ID = id
                };
                break;
            }
            case TypeKind.Function:
            {
                FunctionType header = MemoryMarshal.Read<FunctionType>(span);
                span = span.Slice(Marshal.SizeOf(typeof(FunctionType)));
                
                IL.FunctionType type = new()
                {
                    Kind = kind,
                    ID = id,
                    ReturnType = UnpackTinyType(ref span, resolver),
                    ParameterTypes = new Type[header.parameterCount]
                };

                span = span.Slice((int)(sizeof(ushort) * header.parameterCount));
                
                for (int i = 0; i < header.parameterCount; i++)
                {
                    type.ParameterTypes[i] = UnpackTinyType(ref span, resolver);
                }
                
                opaqueType = type;
                break;
            }
            case TypeKind.Struct:
            {
                StructType header = MemoryMarshal.Read<StructType>(span);
                span = span.Slice(Marshal.SizeOf(typeof(StructType)));
                
                IL.StructType type = new()
                {
                    Kind = kind,
                    ID = id,
                    MemberTypes = new Type[header.memberCount]
                };
                
                span = span.Slice((int)(sizeof(ushort) * header.memberCount));

                for (int i = 0; i < header.memberCount; i++)
                {
                    type.MemberTypes[i] = UnpackTinyType(ref span, resolver);
                }
                
                opaqueType = type;
                break;
            }
        }

        return opaqueType;
    }

    /// <summary>
    /// Unpack a tiny type
    /// </summary>
    public static Type UnpackTinyType(MessageArray<byte> array, TinyTypeResolver resolver)
    {
        unsafe
        {
            ReadOnlySpan<byte> span = new ReadOnlySpan<byte>(array.GetDataStart(), array.Count);
            return UnpackTinyType(ref span, resolver);
        }
    }
}
