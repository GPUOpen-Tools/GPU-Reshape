using System;
using System.Runtime.InteropServices;
using Studio.Models.IL;
using Type = Studio.Models.IL.Type;

namespace GRS.Features.Debug.UIX.ViewModels.Utils;

public class ValueRangeInfo
{
    /// <summary>
    /// Resulting range
    /// </summary>
    public float MinValue =  1e6f;
    public float MaxValue = -1e6f;
}

public static class ValueTypeRangeUtils
{
    /// <summary>
    /// Range a boolean value
    /// </summary>
    public static void RangeBool(BoolType type, ref Span<byte> byteSpan, ValueRangeInfo info)
    {
        info.MinValue = Math.Min(info.MinValue, byteSpan[0]);
        info.MaxValue = Math.Max(info.MaxValue, byteSpan[0]);
        byteSpan = byteSpan.Slice(1);
    }

    /// <summary>
    /// Range an integer value
    /// </summary>
    public static void RangeInt(IntType type, ref Span<byte> byteSpan, ValueRangeInfo info)
    {
        switch (type.BitWidth)
        {
            default:
            {
                return;
            }
            case 1:
            {
                info.MinValue = Math.Min(info.MinValue, byteSpan[0]);
                info.MaxValue = Math.Max(info.MaxValue, byteSpan[0]);
                byteSpan = byteSpan.Slice(1);
                return;
            }
            case 16:
            {
                if (type.Signedness)
                {
                    short value = MemoryMarshal.Read<short>(byteSpan);
                    info.MinValue = Math.Min(info.MinValue, value);
                    info.MaxValue = Math.Max(info.MaxValue, value);
                    byteSpan = byteSpan.Slice(2);
                    return;
                }
                else
                {
                    ushort value = MemoryMarshal.Read<ushort>(byteSpan);
                    info.MinValue = Math.Min(info.MinValue, value);
                    info.MaxValue = Math.Max(info.MaxValue, value);
                    byteSpan = byteSpan.Slice(2);
                    return;
                }
            }
            case 32:
            {
                if (type.Signedness)
                {
                    int value = MemoryMarshal.Read<int>(byteSpan);
                    info.MinValue = Math.Min(info.MinValue, value);
                    info.MaxValue = Math.Max(info.MaxValue, value);
                    byteSpan = byteSpan.Slice(4);
                    return;
                }
                else
                {
                    uint value = MemoryMarshal.Read<uint>(byteSpan);
                    info.MinValue = Math.Min(info.MinValue, value);
                    info.MaxValue = Math.Max(info.MaxValue, value);
                    byteSpan = byteSpan.Slice(4);
                    return;
                }
            }
            case 64:
            {
                if (type.Signedness)
                {
                    Int64 value = MemoryMarshal.Read<Int64>(byteSpan);
                    info.MinValue = Math.Min(info.MinValue, value);
                    info.MaxValue = Math.Max(info.MaxValue, value);
                    byteSpan = byteSpan.Slice(8);
                    return;
                }
                else
                {
                    UInt64 value = MemoryMarshal.Read<UInt64>(byteSpan);
                    info.MinValue = Math.Min(info.MinValue, value);
                    info.MaxValue = Math.Max(info.MaxValue, value);
                    byteSpan = byteSpan.Slice(8);
                    return;
                }
            }
        }
    }

    /// <summary>
    /// Range a floating point value
    /// </summary>
    public static void RangeFP(FPType type, ref Span<byte> byteSpan, ValueRangeInfo info)
    {
        switch (type.BitWidth)
        {
            default:
            {
                return;
            }
            case 16:
            {
                Half value = MemoryMarshal.Read<Half>(byteSpan);
                info.MinValue = Math.Min(info.MinValue, (float)value);
                info.MaxValue = Math.Max(info.MaxValue, (float)value);
                byteSpan = byteSpan.Slice(2);
                return;
            }
            case 32:
            {
                float value = MemoryMarshal.Read<float>(byteSpan);
                info.MinValue = Math.Min(info.MinValue, value);
                info.MaxValue = Math.Max(info.MaxValue, value);
                byteSpan = byteSpan.Slice(4);
                return;
            }
            case 64:
            {
                double value = MemoryMarshal.Read<double>(byteSpan);
                info.MinValue = Math.Min(info.MinValue, (float)value);
                info.MaxValue = Math.Max(info.MaxValue, (float)value);
                byteSpan = byteSpan.Slice(8);
                return;
            }
        }
    }

    /// <summary>
    /// Range an opaque value
    /// </summary>
    public static void RangeValue(Type type, Span<byte> byteSpan, ValueRangeInfo info)
    {
        Span<byte> byteSpanRef = byteSpan;
        RangeValue(type, ref byteSpanRef, info);
    }

    /// <summary>
    /// Range an opaque value
    /// </summary>
    public static void RangeValue(Type type, ref Span<byte> byteSpan, ValueRangeInfo info)
    {
        switch (type.Kind)
        {
            default:
            {
                return;
            }
            case TypeKind.Bool:
            {
                RangeBool((BoolType)type, ref byteSpan, info);
                return;
            }
            case TypeKind.Int:
            {
                RangeInt((IntType)type, ref byteSpan, info);
                return;
            }
            case TypeKind.FP:
            {
                RangeFP((FPType)type, ref byteSpan, info);
                return;
            }
            case TypeKind.Vector:
            {
                var typed = (VectorType)type;

                for (int i = 0; i < typed.Dimension; i++)
                {
                    RangeValue(typed.ContainedType, ref byteSpan, info);
                }
                return;
            }
            case TypeKind.Array:
            {
                var typed = (ArrayType)type;

                for (int i = 0; i < typed.Count; i++)
                {
                    RangeValue(typed.ElementType, ref byteSpan, info);
                }
                return;
            }
            case TypeKind.Matrix:
            {
                var typed = (MatrixType)type;
                
                for (int row = 0; row < typed.Rows; row++)
                {
                    for (int column = 0; column < typed.Columns; column++)
                    {
                        RangeValue(typed.ContainedType, ref byteSpan, info);
                    }
                }
                return;
            }
            case TypeKind.Struct:
            {
                var typed = (StructType)type;
                
                for (int i = 0; i < typed.MemberTypes.Length; i++)
                {
                    RangeValue(typed.MemberTypes[i], ref byteSpan, info);
                }
                return;
            }
        }
    }
}
