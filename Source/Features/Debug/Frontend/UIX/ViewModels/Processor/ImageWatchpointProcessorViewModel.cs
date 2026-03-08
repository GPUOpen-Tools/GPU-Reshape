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
using Studio;
using Studio.Models.IL;

namespace GRS.Features.Debug.UIX.ViewModels.Processor;

public class ImageWatchpointProcessorViewModel : IWatchpointProcessorViewModel
{
    /// <summary>
    /// Process a watchpoint stream, this happens on a separate thread
    /// Must not interact with the UI thread
    /// </summary>
    /// <returns>optional payload data</returns>
    public unsafe object? Process(WatchpointViewModel watchpointViewModel, DebugWatchpointStreamMessage message)
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
        if ((WatchpointDataOrder)message.dataOrder == WatchpointDataOrder.Static)
        {
            bitmap = ProcessStatic(watchpointViewModel, bitmapFormat, message);
        }
        else
        {
            bitmap = ProcessDynamic(watchpointViewModel, bitmapFormat, message);
        }

        // Data span
        Span<uint> dwordSpan = new(message.data.GetDataStart(), message.data.Count / sizeof(uint));

        // TODO: This is a terrible copy, but we don't actually own the stream memory
        return new Payload()
        {
            Image = bitmap,
            Flat = message.Flat,
            WatchpointViewModel = watchpointViewModel,
            DWords = dwordSpan.ToArray()
        };
    }

    /// <summary>
    /// Static processor
    /// </summary>
    private unsafe WriteableBitmap ProcessStatic(WatchpointViewModel watchpointViewModel, PixelFormat bitmapFormat, DebugWatchpointStreamMessage message)
    {
        var imageDisplayViewModel = watchpointViewModel.DisplayViewModel as ImageWatchpointDisplayViewModel;

        // Shared formatting config
        ValueTypeRenderingUtils.FormattingConfig config = GetFormattingConfig(imageDisplayViewModel);
        
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
            // Already gamma encoded
            config.Gamma = 1.0f;
            
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
                uint texel = ValueTypeRenderingUtils.Render255(config, watchpointViewModel.TinyType, 0, dataSpan);

                // Fixed alpha if needed
                texel = ValueTypeRenderingUtils.RenderFixedAlpha255(watchpointViewModel.TinyType, texel);
            
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
    private unsafe WriteableBitmap ProcessDynamic(WatchpointViewModel watchpointViewModel, PixelFormat bitmapFormat, DebugWatchpointStreamMessage message)
    {
        var imageDisplayViewModel = watchpointViewModel.DisplayViewModel as ImageWatchpointDisplayViewModel;

        // Shared formatting config
        ValueTypeRenderingUtils.FormattingConfig config = GetFormattingConfig(imageDisplayViewModel);

        // Deduce the actual safe number of dwords
        uint streamDWordCount = (uint)(message.data.Count / sizeof(uint));
        uint dataDWordCount   = message.dataDynamicCounter * (DynamicWatchpointHeader.DWordCount + message.dataDWordStride);
        uint dwordCount       = Math.Min(streamDWordCount, dataDWordCount);

        // Source dwords
        uint* sourceDWordPtr = (uint*)message.data.GetDataStart();

        // Stride per export
        uint exportDWordStride = DynamicWatchpointHeader.DWordCount + message.dataDWordStride;
        
        // Total export count
        uint exportCount = dwordCount / exportDWordStride;
        
        // Destination buffer
        uint[] dynamicCompositeBuffer = new uint[message.dataStaticWidth * message.dataStaticHeight];

        // Fast path, compressed and scatter memcpy
        if (message.dataFormat != 0)
        {
            // Already gamma encoded
            config.Gamma = 1.0f;
            
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
                    dynamicCompositeBuffer[threadIndex] = ValueTypeRenderingUtils.Repack255(config, sourceDWordPtr[dwordOffset + 1]);
                }
            });
        }
        else
        {
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
                uint texel = ValueTypeRenderingUtils.Render255(config, watchpointViewModel.TinyType, 0, dataSpan);

                // Fixed alpha if needed
                texel = ValueTypeRenderingUtils.RenderFixedAlpha255(watchpointViewModel.TinyType, texel);

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
    public void Install(IWatchpointDisplayViewModel displayViewModel, object payload)
    {
        var typed = (Payload)payload;
        
        if (displayViewModel is ImageWatchpointDisplayViewModel imageDisplayViewModel)
        {
            imageDisplayViewModel.Image = typed.Image;
            
            // Create inspector
            imageDisplayViewModel.Inspector = new Inspector
            {
                DWords = typed.DWords,
                WatchpointViewModel = typed.WatchpointViewModel,
                Flat = typed.Flat
            };
        }
    }

    /// <summary>
    /// Get the formatting config
    /// </summary>
    private static ValueTypeRenderingUtils.FormattingConfig GetFormattingConfig(ImageWatchpointDisplayViewModel? imageDisplayViewModel)
    {
        return new()
        {
            MinValue = imageDisplayViewModel?.MinValue ?? 0.0f,
            MaxValue = imageDisplayViewModel?.MaxValue ?? 1.0f,
            Gamma = (imageDisplayViewModel?.IsSRGB ?? true) ? ValueTypeRenderingUtils.FormattingConfig.DefaultGamma : 1.0f,
            TexelChannelMask = GetTexelChannelMask(imageDisplayViewModel),
            TexelChannelConstant = GetTexelChannelConstant(imageDisplayViewModel)
        };
    }

    /// <summary>
    /// Get the color 255 mask
    /// </summary>
    private static uint GetTexelChannelMask(ImageWatchpointDisplayViewModel? imageDisplayViewModel)
    {
        if (imageDisplayViewModel == null)
        {
            return ~0u;
        }
        
        uint mask = 0x0;
            
        if (imageDisplayViewModel.ColorMask.HasFlag(ColorMask.R))
        {
            mask |= 0xFF;
        }
            
        if (imageDisplayViewModel.ColorMask.HasFlag(ColorMask.G))
        {
            mask |= 0xFFu << 8;
        }
            
        if (imageDisplayViewModel.ColorMask.HasFlag(ColorMask.B))
        {
            mask |= 0xFFu << 16;
        }
            
        if (imageDisplayViewModel.ColorMask.HasFlag(ColorMask.A))
        {
            mask |= 0xFFu << 24;
        }
            
        return mask;
    }

    /// <summary>
    /// Get the color 255 constant
    /// </summary>
    private static uint GetTexelChannelConstant(ImageWatchpointDisplayViewModel? imageDisplayViewModel)
    {
        if (imageDisplayViewModel == null)
        {
            return 0u;
        }
        
        uint constant = 0x0;
        
        if (!imageDisplayViewModel.ColorMask.HasFlag(ColorMask.A))
        {
            constant |= 0xFFu << 24;
        }
            
        return constant;
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
        public required DebugWatchpointStreamMessage.FlatInfo Flat;

        /// <summary>
        /// Owning watchpoint
        /// </summary>
        public required WatchpointViewModel WatchpointViewModel;
        
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
        /// Owning watchpoint
        /// </summary>
        public required WatchpointViewModel WatchpointViewModel;
        
        /// <summary>
        /// Message info
        /// </summary>
        public required DebugWatchpointStreamMessage.FlatInfo Flat;
        
        /// <summary>
        /// Inspect a value
        /// </summary>
        public PixelInspectionRender Inspect(uint x, uint y)
        {
            var imageDisplayViewModel = WatchpointViewModel.DisplayViewModel as ImageWatchpointDisplayViewModel;

            // Shared formatting config
            ValueTypeRenderingUtils.FormattingConfig config = GetFormattingConfig(imageDisplayViewModel);
            
            // Slow path, but it's fine
            try
            {
                if ((WatchpointDataOrder)Flat.dataOrder == WatchpointDataOrder.Static)
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
                        uint texel = ValueTypeRenderingUtils.Render255(config, WatchpointViewModel.TinyType, 0, dataSpan);
                    
                        return new PixelInspectionRender()
                        {
                            Color = TexelToColor(ValueTypeRenderingUtils.RenderFixedAlpha255(WatchpointViewModel.TinyType, texel)),
                            NativeFormatRender = ValueTypeFormattingUtils.FormatValue(WatchpointViewModel.TinyType, dataSpan)
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
        /// Summarize numeric ranges
        /// </summary>
        /// <returns></returns>
        public PixelValueRange SummarizeRange()
        {
            ValueRangeInfo range = new();

            // Slow path, but it's fine
            try
            {
                if ((WatchpointDataOrder)Flat.dataOrder == WatchpointDataOrder.Static)
                {
                    // Static buffers are always fully resident
                    uint exportCount = Flat.dataStaticWidth * Flat.dataStaticHeight;
                    
                    // If there's a data format, just unpack the format
                    if (Flat.dataFormat != 0)
                    {
                        for (int i = 0; i < exportCount; i++)
                        {
                            Color color = TexelToColor(DWords[i]);
                            range.MinValue = Math.Min(range.MinValue, color.R / 255.0f);
                            range.MinValue = Math.Min(range.MinValue, color.G / 255.0f);
                            range.MinValue = Math.Min(range.MinValue, color.B / 255.0f);
                            range.MaxValue = Math.Max(range.MaxValue, color.R / 255.0f);
                            range.MaxValue = Math.Max(range.MaxValue, color.G / 255.0f);
                            range.MaxValue = Math.Max(range.MaxValue, color.B / 255.0f);
                        }
                    }
                    else
                    {
                        for (int i = 0; i < exportCount; i++)
                        {
                            // Offset by data stride
                            int dwordOffset = (int)(i * Flat.dataDWordStride);
            
                            Span<byte> dataSpan = MemoryMarshal.AsBytes(new Span<uint>(
                                DWords, 
                                dwordOffset,
                                (int)Flat.dataDWordStride
                            ));

                            // Range the span
                            ValueTypeRangeUtils.RangeValue(WatchpointViewModel.TinyType, dataSpan, range);
                        }
                    }
                }
                else
                {
                    // Deduce the actual safe number of dwords
                    uint dataDWordCount   = Flat.dataDynamicCounter * (DynamicWatchpointHeader.DWordCount + Flat.dataDWordStride);
                    uint dwordCount       = Math.Min((uint)DWords.Length, dataDWordCount);

                    // Stride per export
                    uint exportDWordStride = DynamicWatchpointHeader.DWordCount + Flat.dataDWordStride;
        
                    // Total export count
                    uint exportCount = dwordCount / exportDWordStride;
                    
                    // Fast path, compressed and scatter memcpy
                    if (Flat.dataFormat != 0)
                    {
                        for (int i = 0; i < exportCount; i++)
                        {
                            int dwordOffset = (int)(i * exportDWordStride);
                            if (dwordOffset + 2 > dwordCount)
                            {
                                break;
                            }
            
                            Color color = TexelToColor(DWords[dwordOffset + 1]);
                            range.MinValue = Math.Min(range.MinValue, color.R / 255.0f);
                            range.MinValue = Math.Min(range.MinValue, color.G / 255.0f);
                            range.MinValue = Math.Min(range.MinValue, color.B / 255.0f);
                            range.MaxValue = Math.Max(range.MaxValue, color.R / 255.0f);
                            range.MaxValue = Math.Max(range.MaxValue, color.G / 255.0f);
                            range.MaxValue = Math.Max(range.MaxValue, color.B / 255.0f);
                        }
                    }
                    else
                    {
                        for (int i = 0; i < exportCount; i++)
                        {
                            int dwordOffset = (int)(i * exportDWordStride);
                            if (dwordOffset + 2 > dwordCount)
                            {
                                break;
                            }

                            Span<byte> dataSpan = MemoryMarshal.AsBytes(new Span<uint>(
                                DWords, 
                                dwordOffset + 1,
                                (int)Flat.dataDWordStride
                            ));
                            
                            // Range the span
                            ValueTypeRangeUtils.RangeValue(WatchpointViewModel.TinyType, dataSpan, range);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Logging.Error($"Failed to range data, {ex}");
            }

            // Setup range
            return new PixelValueRange()
            {
                MinValue = range.MinValue,
                MaxValue = range.MaxValue
            };
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