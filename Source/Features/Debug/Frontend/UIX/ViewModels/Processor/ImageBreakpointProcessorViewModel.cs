using System;
using System.Threading.Tasks;
using Avalonia;
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
    public object? Process(BreakpointViewModel breakpointViewModel, DebugBreakpointStreamMessage message)
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
        if ((BreakpointDataOrder)message.dataOrder == BreakpointDataOrder.Static)
        {
            return ProcessStatic(breakpointViewModel, bitmapFormat, message);
        }
        else
        {
            return ProcessDynamic(breakpointViewModel, bitmapFormat, message);
        }
    }

    /// <summary>
    /// Static processor
    /// </summary>
    private unsafe object? ProcessStatic(BreakpointViewModel breakpointViewModel, PixelFormat bitmapFormat, DebugBreakpointStreamMessage message)
    {
        // Fast path, compressed and ready
        if (message.dataFormat != 0)
        {
            return new WriteableBitmap(
                bitmapFormat, AlphaFormat.Opaque,
                new IntPtr(message.data.GetDataStart()), new PixelSize((int)message.dataStaticWidth, (int)message.dataStaticHeight),
                new Vector(96, 96), (int)(message.dataStaticWidth * message.dataDWordStride * 4)
            );
        }

        // TODO: Implement
        return null;
    }

    /// <summary>
    /// Dynamic processor
    /// </summary>
    private unsafe object? ProcessDynamic(BreakpointViewModel breakpointViewModel, PixelFormat bitmapFormat, DebugBreakpointStreamMessage message)
    {
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
                MaxValue = 1.0f
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

                uint threadIndex = sourceDWordPtr[dwordOffset];
                if (threadIndex < dynamicCompositeBuffer.Length)
                {
                    dynamicCompositeBuffer[threadIndex] = texel;
                }
            });
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
        if (displayViewModel is ImageBreakpointDisplayViewModel imageDisplayViewModel)
        {
            imageDisplayViewModel.Image = (WriteableBitmap)payload;
        }
    }
}