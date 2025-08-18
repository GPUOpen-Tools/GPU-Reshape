using System;
using Avalonia;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using Avalonia.Threading;
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
    public unsafe object? Process(DebugBreakpointStreamMessage message)
    {
        var format = (Format)message.dataFormat;

        // Right now just choose it based on the compression format
        PixelFormat bitmapFormat;
        switch (format)
        {
            default:
                return null;
            case Format.RGBA8:
                bitmapFormat = PixelFormat.Rgba8888;
                break;
        }

        // Ignore empty images
        if (message.dataStaticWidth == 0 || message.dataStaticHeight == 0)
        {
            return null;
        }
        
        return new WriteableBitmap(
            bitmapFormat, AlphaFormat.Opaque,
            new IntPtr(message.data.GetDataStart()), new PixelSize((int)message.dataStaticWidth, (int)message.dataStaticHeight),
            new Vector(96, 96), (int)(message.dataStaticWidth * message.dataDWordStride * 4)
        );
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