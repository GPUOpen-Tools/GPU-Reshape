using System;
using System.Runtime.InteropServices;
using Studio.Models.IL;
using Type = Studio.Models.IL.Type;

namespace GRS.Features.Debug.UIX.ViewModels.Utils;

public static class ValueTypeRenderingUtils
{
    /// <summary>
    /// All placeholder hacks, just to start somewhere
    /// </summary>
    
    public struct FormattingConfig
    {
        /// <summary>
        /// Default SRGB gamma
        /// </summary>
        public static float DefaultGamma = 0.4545454545f;
        
        /// <summary>
        /// Minimum render value
        /// </summary>
        public required float MinValue;
        
        /// <summary>
        /// Maximum render value
        /// </summary>
        public required float MaxValue;

        /// <summary>
        /// Channel mask
        /// </summary>
        public required uint TexelChannelMask;

        /// <summary>
        /// Channel constants
        /// </summary>
        public required uint TexelChannelConstant;

        /// <summary>
        /// Gamma to apply
        /// </summary>
        public required float Gamma;

        /// <summary>
        /// Is this trivial formatting?
        /// I.e., is memcpy enough?
        /// </summary>
        /// <returns></returns>
        public bool IsTrivial()
        {
            return MinValue < 1e-4f && Math.Abs(MaxValue - 1.0f) < 1e-4f &&
                   Math.Abs(Gamma - DefaultGamma) < 1e-4f &&
                   TexelChannelMask == ~0u;
        }
    }

    /// <summary>
    /// Render a value to a 255 format
    /// </summary>
    public static uint Render255(FormattingConfig config, Type type, int shl, Span<byte> byteSpan)
    {
        Span<byte> byteSpanRef = byteSpan;
        return Render255(config, type, shl, ref byteSpanRef) & config.TexelChannelMask;
    }

    /// <summary>
    /// Render a value to a 255 format
    /// </summary>
    public static uint Render255(FormattingConfig config, Type type, int shl, ref Span<byte> byteSpan)
    {
        switch (type.Kind)
        {
            default:
            {
                return 0;
            }
            case TypeKind.Bool:
            {
                return RenderBool255(config, (BoolType)type, shl, ref byteSpan);
            }
            case TypeKind.Int:
            {
                return RenderInt255(config, (IntType)type, shl, ref byteSpan);
            }
            case TypeKind.FP:
            {
                return RenderFP255(config, (FPType)type, shl, ref byteSpan);
            }
            case TypeKind.Vector:
            {
                var typed = (VectorType)type;

                uint value = 0;
                for (int i = 0; i < Math.Min(typed.Dimension, 4); i++)
                {
                    value |= Render255(config, typed.ContainedType, shl + 8 * i, ref byteSpan);
                }

                return value;
            }
            case TypeKind.Array:
            {
                var typed = (ArrayType)type;

                uint value = 0;
                for (int i = 0; i < Math.Min(typed.Count, 4); i++)
                {
                    value |= Render255(config, typed.ElementType, shl + 8 * i, ref byteSpan);
                }

                return value;
            }
            case TypeKind.Struct:
            {
                var typed = (StructType)type;
                
                uint value = 0;
                for (int i = 0; i < Math.Min(typed.MemberTypes.Length, 4); i++)
                {
                    value |= Render255(config, typed.MemberTypes[i], shl + 8 * i, ref byteSpan);
                }

                return value;
            }
        }
    }
    
    /// <summary>
    /// Render a fixed alpha channel if none present
    /// </summary>
    public static uint RenderFixedAlpha255(Type type, uint render255)
    {
        uint alphaValue = 0;
        
        switch (type.Kind)
        {
            default:
            {
                alphaValue = 0xFFu << 24;
                break;
            }
            case TypeKind.Vector:
            {
                var typed = (VectorType)type;
                if (typed.Dimension < 4)
                {
                    alphaValue = 0xFFu << 24;
                }
                break;
            }
            case TypeKind.Array:
            {
                var typed = (ArrayType)type;
                if (typed.Count < 4)
                {
                    alphaValue = 0xFFu << 24;
                }
                break;
            }
            case TypeKind.Struct:
            {
                var typed = (StructType)type;
                if (typed.MemberTypes.Length < 4)
                {
                    alphaValue = 0xFFu << 24;
                }
                break;
            }
        }

        return render255 | alphaValue;
    }
    
    /// <summary>
    /// Render a boolean value
    /// </summary>
    public static uint RenderBool255(FormattingConfig config, BoolType type, int shl, ref Span<byte> byteSpan)
    {
        uint value = byteSpan[0] * 255u;
        byteSpan = byteSpan.Slice(1);
        return value << shl;
    }

    /// <summary>
    /// Render an integer value
    /// </summary>
    public static uint RenderInt255(FormattingConfig config, IntType type, int shl, ref Span<byte> byteSpan)
    {
        switch (type.BitWidth)
        {
            default:
            {
                return 0;
            }
            case 1:
            {
                uint value = byteSpan[0];
                byteSpan = byteSpan.Slice(1);
                return Pack(config, value, shl);
            }
            case 16:
            {
                if (type.Signedness)
                {
                    short value = MemoryMarshal.Read<short>(byteSpan);
                    byteSpan = byteSpan.Slice(2);
                    return Pack(config, (uint)value, shl);
                }
                else
                {
                    ushort value = MemoryMarshal.Read<ushort>(byteSpan);
                    byteSpan = byteSpan.Slice(2);
                    return Pack(config, value, shl);
                }
            }
            case 32:
            {
                if (type.Signedness)
                {
                    int value = MemoryMarshal.Read<int>(byteSpan);
                    byteSpan = byteSpan.Slice(4);
                    return Pack(config, (uint)value, shl);
                }
                else
                {
                    uint value = MemoryMarshal.Read<uint>(byteSpan);
                    byteSpan = byteSpan.Slice(4);
                    return Pack(config, value, shl);
                }
            }
            case 64:
            {
                if (type.Signedness)
                {
                    Int64 value = MemoryMarshal.Read<Int64>(byteSpan);
                    byteSpan = byteSpan.Slice(8);
                    return Pack(config, (uint)value, shl);
                }
                else
                {
                    UInt64 value = MemoryMarshal.Read<UInt64>(byteSpan);
                    byteSpan = byteSpan.Slice(8);
                    return Pack(config, (uint)value, shl);
                }
            }
        }
    }

    /// <summary>
    /// Render a floating point value
    /// </summary>
    public static uint RenderFP255(FormattingConfig config, FPType type, int shl, ref Span<byte> byteSpan)
    {
        switch (type.BitWidth)
        {
            default:
            {
                return 0;
            }
            case 16:
            {
                Half value = MemoryMarshal.Read<Half>(byteSpan);
                byteSpan = byteSpan.Slice(2);
                return Pack(config, (float)value, shl);
            }
            case 32:
            {
                float value = MemoryMarshal.Read<float>(byteSpan);
                byteSpan = byteSpan.Slice(4);
                return Pack(config, value, shl);
            }
            case 64:
            {
                double value = MemoryMarshal.Read<double>(byteSpan);
                byteSpan = byteSpan.Slice(8);
                return Pack(config, (float)value, shl);
            }
        }
    }

    /// <summary>
    /// Repack a 255 value with new formatting rules
    /// </summary>
    public static uint Repack255(FormattingConfig config, uint texel)
    {
        texel = Pack(config, (texel & 0xFF) / 255.0f, 0) |
                Pack(config, ((texel >> 8) & 0xFF) / 255.0f, 8) |
                Pack(config, ((texel >> 16) & 0xFF) / 255.0f, 16) |
                Pack(config, ((texel >> 24) & 0xFF) / 255.0f, 24);
        
        return texel & config.TexelChannelMask | config.TexelChannelConstant;
    }

    /// <summary>
    /// Pack a single fp value
    /// </summary>
    private static uint Pack(FormattingConfig config, float value, int shl)
    {
        // Gamma
        value = (float)Math.Pow(value, config.Gamma);
        return (uint)(Math.Min(Math.Max(0, value - config.MinValue) / (config.MaxValue - config.MinValue), 1.0f) * 255) << shl;
    }

    /// <summary>
    /// Pack a single int value
    /// </summary>
    private static uint Pack(FormattingConfig config, uint value, int shl)
    {
        return (uint)(Math.Min(Math.Max(0, value - config.MinValue) / (config.MaxValue - config.MinValue), 1.0f) * 255) << shl;
    }
}
