using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.ViewModels.Utils;
using Message.CLR;
using Studio.Models.IL;

namespace GRS.Features.Debug.UIX.ViewModels.Processor;

public class ImageBreakpointProcessorViewModel : IBreakpointProcessorViewModel
{
    /// <summary>
    /// Process a breakpoint stream, this happens on a separate thread
    /// Must not interact with the UI thread
    /// </summary>
    /// <returns>optional payload data</returns>
    public unsafe object? Process(BreakpointViewModel breakpointViewModel, DebugBreakpointStreamMessage message)
    {
        // Ignore empty images
        if (message.dataStaticWidth == 0 || message.dataStaticHeight == 0)
        {
            return null;
        }

        // Right now just choose it based on the compression format
        PixelFormat bitmapFormat;
        switch ((Format)message.dataFormat)
        {
            default:
                throw new NotImplementedException($"Unimplemented format {message.dataFormat}");
            case Format.None:
                bitmapFormat = default;
                break;
            case Format.RGBA8:
                bitmapFormat = PixelFormat.Rgba8888;
                break;
        }

        // Handle processing
        WriteableBitmap bitmap;
        if ((BreakpointDataOrder)message.dataOrder == BreakpointDataOrder.Static)
        {
            bitmap = ProcessStatic(breakpointViewModel, bitmapFormat, message);
        }
        else
        {
            bitmap = ProcessDynamic(breakpointViewModel, bitmapFormat, message);
        }

        // Data span
        Span<uint> dwordSpan = new(message.data.GetDataStart(), message.data.Count / sizeof(uint));

        // TODO: This is a terrible copy, but we don't actually own the stream memory
        return new Payload()
        {
            Image = bitmap,
            Flat = message.Flat,
            BreakpointViewModel = breakpointViewModel,
            DWords = dwordSpan.ToArray()
        };
    }

    /// <summary>
    /// Static processor
    /// </summary>
    private unsafe WriteableBitmap ProcessStatic(BreakpointViewModel breakpointViewModel, PixelFormat bitmapFormat, DebugBreakpointStreamMessage message)
    {
        var imageDisplayViewModel = breakpointViewModel.DisplayViewModel as ImageBreakpointDisplayViewModel;
        
        // Shared formatting config
        ValueTypeRenderingUtils.FormattingConfig config = new()
        {
            MinValue = imageDisplayViewModel?.MinValue ?? 0.0f,
            MaxValue = imageDisplayViewModel?.MaxValue ?? 1.0f
        };
        
        // Fast path, compressed and ready
        if (message.dataFormat != 0 && config.IsTrivial())
        {
            return new WriteableBitmap(
                bitmapFormat, AlphaFormat.Opaque,
                new IntPtr(message.data.GetDataStart()), new PixelSize((int)message.dataStaticWidth, (int)message.dataStaticHeight),
                new Vector(96, 96), (int)(message.dataStaticWidth * message.dataDWordStride * 4)
            );
        }

        // Source dwords
        uint* sourceDWordPtr = (uint*)message.data.GetDataStart();

        // Static buffers are always fully resident
        uint exportCount = message.dataStaticWidth * message.dataStaticHeight;
        
        // Destination buffer
        uint[] dynamicCompositeBuffer = new uint[exportCount];

        // If there's a data format, don't render the entire thing, just re-pack
        if (message.dataFormat != 0)
        {
            // Parallelize composition
            Parallel.For(0, exportCount, i =>
            {
                dynamicCompositeBuffer[i] = ValueTypeRenderingUtils.Repack255(config, sourceDWordPtr[i]);
            });
        }
        else
        {
            // Parallelize composition
            Parallel.For(0, exportCount, i =>
            {
                int dwordOffset = (int)(i * message.dataDWordStride);
            
                Span<byte> dataSpan = new(
                    (byte*)(sourceDWordPtr + dwordOffset),
                    (int)(message.dataDWordStride * 4)
                );

                // Render using the tiny type, must exist at this point
                uint texel = ValueTypeRenderingUtils.Render255(config, breakpointViewModel.TinyType, 0, ref dataSpan);

                // Fixed alpha if needed
                texel = ValueTypeRenderingUtils.RenderFixedAlpha255(breakpointViewModel.TinyType, texel);
            
                dynamicCompositeBuffer[i] = texel;
            });
        }

        // Create image
        fixed (uint* ptr = dynamicCompositeBuffer)
        {
            return new WriteableBitmap(
                PixelFormat.Rgba8888, AlphaFormat.Opaque,
                new IntPtr(ptr), new PixelSize((int)message.dataStaticWidth, (int)message.dataStaticHeight),
                new Vector(96, 96), (int)(message.dataStaticWidth * 4)
            );
        }
    }

    /// <summary>
    /// Dynamic processor
    /// </summary>
    private unsafe WriteableBitmap ProcessDynamic(BreakpointViewModel breakpointViewModel, PixelFormat bitmapFormat, DebugBreakpointStreamMessage message)
    {
        var imageDisplayViewModel = breakpointViewModel.DisplayViewModel as ImageBreakpointDisplayViewModel;

        // Deduce the actual safe number of dwords
        uint streamDWordCount = (uint)(message.data.Count / sizeof(uint));
        uint dataDWordCount   = message.dataDynamicCounter * (DynamicBreakpointHeader.DWordCount + message.dataDWordStride);
        uint dwordCount       = Math.Min(streamDWordCount, dataDWordCount);

        // Source dwords
        uint* sourceDWordPtr = (uint*)message.data.GetDataStart();

        // Stride per export
        uint exportDWordStride = DynamicBreakpointHeader.DWordCount + message.dataDWordStride;
        
        // Total export count
        uint exportCount = dwordCount / exportDWordStride;
        
        // Destination buffer
        uint[] dynamicCompositeBuffer = new uint[message.dataStaticWidth * message.dataStaticHeight];

        // Fast path, compressed and scatter memcpy
        if (message.dataFormat != 0)
        {
            // Parallelize composition
            Parallel.For(0, exportCount, i =>
            {
                int dwordOffset = (int)(i * exportDWordStride);
                if (dwordOffset + 2 > dwordCount)
                {
                    return;
                }
            
                uint threadIndex = sourceDWordPtr[dwordOffset];
                if (threadIndex < dynamicCompositeBuffer.Length)
                {
                    dynamicCompositeBuffer[threadIndex] = sourceDWordPtr[dwordOffset + 1];
                }
            });
        }
        else
        {
            // Shared formatting config
            ValueTypeRenderingUtils.FormattingConfig config = new()
            {
                MinValue = imageDisplayViewModel?.MinValue ?? 0.0f,
                MaxValue = imageDisplayViewModel?.MaxValue ?? 1.0f
            };
            
            // Parallelize composition
            Parallel.For(0, exportCount, i =>
            {
                int dwordOffset = (int)(i * exportDWordStride);
                if (dwordOffset + 2 > dwordCount)
                {
                    return;
                }
                
                Span<byte> dataSpan = new(
                    (byte*)(sourceDWordPtr + dwordOffset + 1),
                    (int)(message.dataDWordStride * 4)
                );

                // Render using the tiny type, must exist at this point
                uint texel = ValueTypeRenderingUtils.Render255(config, breakpointViewModel.TinyType, 0, ref dataSpan);

                // Fixed alpha if needed
                texel = ValueTypeRenderingUtils.RenderFixedAlpha255(breakpointViewModel.TinyType, texel);

                uint threadIndex = sourceDWordPtr[dwordOffset];
                if (threadIndex < dynamicCompositeBuffer.Length)
                {
                    dynamicCompositeBuffer[threadIndex] = texel;
                }
            });
            
            // Assume 255
            bitmapFormat = PixelFormat.Rgba8888;
        }

        // Create image
        fixed (uint* ptr = dynamicCompositeBuffer)
        {
            return new WriteableBitmap(
                bitmapFormat, AlphaFormat.Opaque,
                new IntPtr(ptr), new PixelSize((int)message.dataStaticWidth, (int)message.dataStaticHeight),
                new Vector(96, 96), (int)(message.dataStaticWidth * 4)
            );
        }
    }
    
    /// <summary>
    /// Install the payload on the UI thread
    /// </summary>
    public void Install(IBreakpointDisplayViewModel displayViewModel, object payload)
    {
        var typed = (Payload)payload;
        
        if (displayViewModel is ImageBreakpointDisplayViewModel imageDisplayViewModel)
        {
            imageDisplayViewModel.Image = typed.Image;
            
            // Create inspector
            imageDisplayViewModel.Inspector = new Inspector
            {
                DWords = typed.DWords,
                BreakpointViewModel = typed.BreakpointViewModel,
                Flat = typed.Flat
            };
        }
    }

    private class Payload
    {
        /// <summary>
        /// Rendered image
        /// </summary>
        public required WriteableBitmap Image;

        /// <summary>
        /// Message info
        /// </summary>
        public required DebugBreakpointStreamMessage.FlatInfo Flat;

        /// <summary>
        /// Owning breakpoint
        /// </summary>
        public required BreakpointViewModel BreakpointViewModel;
        
        /// <summary>
        /// Data copy
        /// </summary>
        public required uint[] DWords;
    }

    private class Inspector : IImageInspector
    {
        /// <summary>
        /// All dwords
        /// </summary>
        public required uint[] DWords;
        
        /// <summary>
        /// Owning breakpoint
        /// </summary>
        public required BreakpointViewModel BreakpointViewModel;
        
        /// <summary>
        /// Message info
        /// </summary>
        public required DebugBreakpointStreamMessage.FlatInfo Flat;
        
        /// <summary>
        /// Inspect a value
        /// </summary>
        public PixelInspectionRender Inspect(uint x, uint y)
        {
            var imageDisplayViewModel = BreakpointViewModel.DisplayViewModel as ImageBreakpointDisplayViewModel;

            // Shared formatting config
            ValueTypeRenderingUtils.FormattingConfig config = new()
            {
                MinValue = imageDisplayViewModel?.MinValue ?? 0.0f,
                MaxValue = imageDisplayViewModel?.MaxValue ?? 1.0f
            };

            // Slow path, but it's fine
            try
            {
                if ((BreakpointDataOrder)Flat.dataOrder == BreakpointDataOrder.Static)
                {
                    // Static ordering
                    uint index = y * Flat.dataStaticWidth + x;
                    uint data  = DWords[index];
                
                    // Compressed?
                    if (Flat.dataFormat != 0)
                    {
                        Color color = TexelToColor(data);
                        return new PixelInspectionRender()
                        {
                            Color = color,
                            NativeFormatRender = $"R:{color.R} G:{color.G} B:{color.B} A:{color.A}"
                        };
                    }
                    else
                    {
                        // Offset by data stride
                        int dwordOffset = (int)(index * Flat.dataDWordStride);
            
                        Span<byte> dataSpan = MemoryMarshal.AsBytes(new Span<uint>(DWords, dwordOffset, (int)Flat.dataDWordStride));

                        // Render using the tiny type, must exist at this point
                        uint texel = ValueTypeRenderingUtils.Render255(config, BreakpointViewModel.TinyType, 0, dataSpan);
                    
                        return new PixelInspectionRender()
                        {
                            Color = TexelToColor(ValueTypeRenderingUtils.RenderFixedAlpha255(BreakpointViewModel.TinyType, texel)),
                            NativeFormatRender = ValueTypeFormattingUtils.FormatValue(BreakpointViewModel.TinyType, dataSpan)
                        };
                    }
                }
                else
                {
                    // TODO: ...
                    return new PixelInspectionRender { Color = Colors.Transparent, NativeFormatRender = "Not Implemented" };
                }
            }
            catch (Exception)
            {
                return new PixelInspectionRender { Color = Colors.Transparent, NativeFormatRender = "Failed" };
            }
        }

        /// <summary>
        /// Convert a 255 texel to color
        /// </summary>
        private Color TexelToColor(uint data)
        {
            byte r = (byte)((data >> 0) & 0xFF);
            byte g = (byte)((data >> 8) & 0xFF);
            byte b = (byte)((data >> 16) & 0xFF);
            byte a = (byte)((data >> 24) & 0xFF);

            return Color.FromArgb(a, r, g, b);
        }
    }
}