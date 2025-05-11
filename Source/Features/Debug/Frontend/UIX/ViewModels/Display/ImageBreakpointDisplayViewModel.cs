using System;
using ReactiveUI;
using Avalonia.Media;
using GRS.Features.Debug.UIX.Models;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

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
    /// Shader property
    /// </summary>
    public ShaderViewModel? ShaderProperty { get; set; }

    /// <summary>
    /// Should the aspect ratio be maintained? i.e., stretch or not
    /// </summary>
    public bool MaintainAspectRatio
    {
        get => _maintainAspectRatio;
        set => this.RaiseAndSetIfChanged(ref _maintainAspectRatio, value);
    }

    /// <summary>
    /// Should we compress the image for performance?
    /// </summary>
    public bool Compress
    {
        get => _compress;
        set => this.RaiseAndSetIfChanged(ref _compress, value);
    }

    public ImageBreakpointDisplayViewModel()
    {
        // Values that require reinstrumentation
        this.WhenAnyValue(x => x.Compress)
            .Subscribe(_ => OnInstrumentChanged());
    }

    /// <summary>
    /// Invoked whenever instrumentation needs changing
    /// </summary>
    private void OnInstrumentChanged()
    {
        ShaderProperty?.EnqueueFirstParentBus();
    }

    /// <summary>
    /// Apply all local breakpoint instrumentation data
    /// </summary>
    /// <param name="config"></param>
    public void ApplyBreakpointConfig(BreakpointConfig config)
    {
        if (_compress)
        {
            config.Flags |= BreakpointFlag.AllowImageFPUNorm8888Compression;
        }
    }
    
    /// <summary>
    /// Internal image
    /// </summary>
    private IImage? _image = null;

    /// <summary>
    /// Internal aspect ratio state
    /// </summary>
    private bool _maintainAspectRatio = true;
    
    /// <summary>
    /// Internal compress state
    /// </summary>
    private bool _compress = true;
}
