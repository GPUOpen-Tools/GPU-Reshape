using System;
using System.Windows.Input;
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
    /// Optional pixel inspector
    /// </summary>
    public IImageInspector? Inspector { get; set; }

    /// <summary>
    /// Shader property
    /// </summary>
    public ShaderViewModel? ShaderProperty
    {
        get => _shaderProperty;
        set => this.RaiseAndSetIfChanged(ref _shaderProperty, value);
    }

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

    /// <summary>
    /// Is this view locked?
    /// </summary>
    public bool LockToContent
    {
        get => _lockToContent;
        set => this.RaiseAndSetIfChanged(ref _lockToContent, value);
    }

    /// <summary>
    /// Current min display value
    /// </summary>
    public float MinValue
    {
        get => _minValue;
        set => this.RaiseAndSetIfChanged(ref _minValue, value);
    }

    /// <summary>
    /// Current max display value
    /// </summary>
    public float MaxValue
    {
        get => _maxValue;
        set => this.RaiseAndSetIfChanged(ref _maxValue, value);
    }

    /// <summary>
    /// Current pixel decoration for status rendering
    /// </summary>
    public string PixelDecoration
    {
        get => _pixelDecoration;
        set => this.RaiseAndSetIfChanged(ref _pixelDecoration, value);
    }

    /// <summary>
    /// Current decoration color
    /// </summary>
    public IBrush PixelColor
    {
        get => _pixelColor;
        set => this.RaiseAndSetIfChanged(ref _pixelColor, value);
    }

    /// <summary>
    /// Color channel mask
    /// </summary>
    public ColorMask ColorMask
    {
        get => _colorMask;
        set => this.RaiseAndSetIfChanged(ref _colorMask, value);
    }
    
    /// <summary>
    /// Toggle command
    /// </summary>
    public ICommand ToggleColorMaskCommand { get; }

    public ImageBreakpointDisplayViewModel()
    {
        // Values that require reinstrumentation
        this.WhenAnyValue(x => x.Compress, x => x.ShaderProperty)
            .Subscribe(_ => OnInstrumentChanged());

        // Create color mask command
        ToggleColorMaskCommand = ReactiveCommand.Create<ColorMask>(flag =>
        {
            if (ColorMask.HasFlag(flag))
            {
                ColorMask &= ~flag;
            }
            else
            {
                ColorMask |= flag;
            }
        });
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
    /// Internal property
    /// </summary>
    private ShaderViewModel? _shaderProperty;

    /// <summary>
    /// Internal aspect ratio state
    /// </summary>
    private bool _maintainAspectRatio = true;

    /// <summary>
    /// Internal compress state
    /// </summary>
    private bool _compress = true;

    /// <summary>
    /// Internal lock state
    /// </summary>
    private bool _lockToContent = true;

    /// <summary>
    /// Internal min display
    /// </summary>
    private float _minValue = 0.0f;
    
    /// <summary>
    /// Internal max display
    /// </summary>
    private float _maxValue = 1.0f;

    /// <summary>
    /// Internal decoration state
    /// </summary>
    private string _pixelDecoration;
    
    /// <summary>
    /// Internal decoration state
    /// </summary>
    private IBrush _pixelColor;

    /// <summary>
    /// Internal mask state
    /// </summary>
    private ColorMask _colorMask = ColorMask.All;
}
