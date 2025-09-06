using Avalonia.Media;

namespace GRS.Features.Debug.UIX.ViewModels;

public struct PixelInspectionRender
{
    /// <summary>
    /// The rendered format
    /// </summary>
    public required string NativeFormatRender;
    
    /// <summary>
    /// Debugging color
    /// </summary>
    public required Color Color;
}

public interface IImageInspector
{
    /// <summary>
    /// Inspect a pixel value
    /// </summary>
    public PixelInspectionRender Inspect(uint x, uint y);
}
