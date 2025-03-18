using Avalonia.Media;
using ReactiveUI;

namespace GRS.Features.Debug.UIX.ViewModels;

public class ImageBreakpointDisplayViewModel : ReactiveObject, IBreakpointDisplayViewModel
{
    /// <summary>
    /// Streamed image data
    /// </summary>
    public IImage? Image
    {
        get => _image;
        set => this.RaiseAndSetIfChanged(ref _image, value);
    }

    /// <summary>
    /// Framerate of the streamed data
    /// </summary>
    public float FrameRate
    {
        get => _frameRate;
        set => this.RaiseAndSetIfChanged(ref _frameRate, value);
    }

    /// <summary>
    /// Last time it was requested
    /// </summary>
    public long LastTimeStamp = 0;
    
    /// <summary>
    /// Internal image
    /// </summary>
    private IImage? _image = null;
    
    /// <summary>
    /// Internal frame rate
    /// </summary>
    private float _frameRate = 0f;
}
