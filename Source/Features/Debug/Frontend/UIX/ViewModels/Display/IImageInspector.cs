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

public struct PixelValueRange
{
    /// <summary>
    /// Min value summarized
    /// </summary>
    public required float MinValue;
    
    /// <summary>
    /// Max value summarized
    /// </summary>
    public required float MaxValue;
}

public interface IImageInspector
{
    /// <summary>
    /// Inspect a pixel value
    /// </summary>
    public PixelInspectionRender Inspect(uint x, uint y);

    /// <summary>
    /// Get the numeric range of the image
    /// </summary>
    public PixelValueRange SummarizeRange();
}
