using System;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Text;
using Studio.Models.IL;
using Type = Studio.Models.IL.Type;

namespace GRS.Features.Debug.UIX.ViewModels.Utils;

public static class ValueTypeFormattingUtils
{
    /// <summary>
    /// Format a boolean value
    /// </summary>
    public static string FormatBool(BoolType type, ref Span<byte> byteSpan)
    {
        string value = byteSpan[0] == 1 ? "true" : "false";
        byteSpan = byteSpan.Slice(1);
        return value;
    }

    /// <summary>
    /// Format an integer value
    /// </summary>
    public static string FormatInt(IntType type, ref Span<byte> byteSpan)
    {
        switch (type.BitWidth)
        {
            default:
            {
                return string.Empty;
            }
            case 1:
            {
                string value = byteSpan[0] == 1 ? "1" : "0";
                byteSpan = byteSpan.Slice(1);
                return value;
            }
            case 16:
            {
                if (type.Signedness)
                {
                    short value = MemoryMarshal.Read<short>(byteSpan);
                    byteSpan = byteSpan.Slice(2);
                    return value.ToString();
                }
                else
                {
                    ushort value = MemoryMarshal.Read<ushort>(byteSpan);
                    byteSpan = byteSpan.Slice(2);
                    return value.ToString();
                }
            }
            case 32:
            {
                if (type.Signedness)
                {
                    int value = MemoryMarshal.Read<int>(byteSpan);
                    byteSpan = byteSpan.Slice(4);
                    return value.ToString();
                }
                else
                {
                    uint value = MemoryMarshal.Read<uint>(byteSpan);
                    byteSpan = byteSpan.Slice(4);
                    return value.ToString();
                }
            }
            case 64:
            {
                if (type.Signedness)
                {
                    Int64 value = MemoryMarshal.Read<Int64>(byteSpan);
                    byteSpan = byteSpan.Slice(8);
                    return value.ToString();
                }
                else
                {
                    UInt64 value = MemoryMarshal.Read<UInt64>(byteSpan);
                    byteSpan = byteSpan.Slice(8);
                    return value.ToString();
                }
            }
        }
    }

    /// <summary>
    /// Format a floating point value
    /// </summary>
    public static string FormatFP(FPType type, ref Span<byte> byteSpan)
    {
        switch (type.BitWidth)
        {
            default:
            {
                return string.Empty;
            }
            case 16:
            {
                Half value = MemoryMarshal.Read<Half>(byteSpan);
                byteSpan = byteSpan.Slice(2);
                return value.ToString();
            }
            case 32:
            {
                float value = MemoryMarshal.Read<float>(byteSpan);
                byteSpan = byteSpan.Slice(4);
                return value.ToString(CultureInfo.InvariantCulture);
            }
            case 64:
            {
                double value = MemoryMarshal.Read<double>(byteSpan);
                byteSpan = byteSpan.Slice(8);
                return value.ToString(CultureInfo.InvariantCulture);
            }
        }
    }

    /// <summary>
    /// Format an opaque value
    /// </summary>
    public static string FormatValue(Type type, Span<byte> byteSpan)
    {
        Span<byte> byteSpanRef = byteSpan;
        return FormatValue(type, ref byteSpanRef);
    }

    /// <summary>
    /// Format an opaque value
    /// </summary>
    public static string FormatValue(Type type, ref Span<byte> byteSpan)
    {
        switch (type.Kind)
        {
            default:
            {
                return type.Kind.ToString();
            }
            case TypeKind.Bool:
            {
                return FormatBool((BoolType)type, ref byteSpan);
            }
            case TypeKind.Int:
            {
                return FormatInt((IntType)type, ref byteSpan);
            }
            case TypeKind.FP:
            {
                return FormatFP((FPType)type, ref byteSpan);
            }
            case TypeKind.Vector:
            {
                var typed = (VectorType)type;

                StringBuilder builder = new();

                for (int i = 0; i < typed.Dimension; i++)
                {
                    builder.Append(FormatValue(typed.ContainedType, ref byteSpan));
                    builder.Append(' ');
                }

                return builder.ToString();
            }
            case TypeKind.Array:
            {
                var typed = (ArrayType)type;
                
                StringBuilder builder = new();

                for (int i = 0; i < typed.Count; i++)
                {
                    builder.Append(FormatValue(typed.ElementType, ref byteSpan));
                    builder.Append(' ');
                }
                
                return builder.ToString();
            }
            case TypeKind.Matrix:
            {
                var typed = (MatrixType)type;
                
                StringBuilder builder = new();

                for (int row = 0; row < typed.Rows; row++)
                {
                    LooseTreeItemViewModel rowItem = new() { Text = $"Row {row}" };
                    
                    for (int column = 0; column < typed.Columns; column++)
                    {
                        builder.Append(FormatValue(typed.ContainedType, ref byteSpan));
                        builder.Append(' ');
                    }
                    
                    builder.Append(" - ");
                }
                
                return builder.ToString();
            }
            case TypeKind.Struct:
            {
                var typed = (StructType)type;
                
                StringBuilder builder = new();

                for (int i = 0; i < typed.MemberTypes.Length; i++)
                {
                    builder.Append(FormatValue(typed.MemberTypes[i], ref byteSpan));
                    builder.Append(' ');
                }
                
                return builder.ToString();
            }
        }
    }
}
