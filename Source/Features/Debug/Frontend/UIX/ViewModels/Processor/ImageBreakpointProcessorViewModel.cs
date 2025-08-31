using System;
using System.Threading.Tasks;
using Avalonia;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using GRS.Features.Debug.UIX.Models;
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
    public object? Process(DebugBreakpointStreamMessage message)
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
                return null;
            case Format.RGBA8:
                bitmapFormat = PixelFormat.Rgba8888;
                break;
        }

        // Handle processing
        if ((BreakpointDataOrder)message.dataOrder == BreakpointDataOrder.Static)
        {
            return ProcessStatic(bitmapFormat, message);
        }
        else
        {
            return ProcessDynamic(bitmapFormat, message);
        }
    }

    /// <summary>
    /// Static processor
    /// </summary>
    private unsafe object? ProcessStatic(PixelFormat bitmapFormat, DebugBreakpointStreamMessage message)
    {
        return new WriteableBitmap(
            bitmapFormat, AlphaFormat.Opaque,
            new IntPtr(message.data.GetDataStart()), new PixelSize((int)message.dataStaticWidth, (int)message.dataStaticHeight),
            new Vector(96, 96), (int)(message.dataStaticWidth * message.dataDWordStride * 4)
        );
    }

    /// <summary>
    /// Dynamic processor
    /// </summary>
    private unsafe object? ProcessDynamic(PixelFormat bitmapFormat, DebugBreakpointStreamMessage message)
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

        // Parallelize composition
        Parallel.For(0, exportCount, i =>
        {
            int dwordOffset = (int)(i * exportDWordStride);
            if (dwordOffset + 2 > dwordCount)
            {
                return;
            }
            
            uint threadIndex = sourceDWordPtr[dwordOffset];
            uint texel = sourceDWordPtr[dwordOffset + 1];
            dynamicCompositeBuffer[threadIndex] = texel;
        });

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