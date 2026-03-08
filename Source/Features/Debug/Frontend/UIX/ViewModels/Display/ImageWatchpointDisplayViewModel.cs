using System;
using System.Collections.ObjectModel;
using System.Windows.Input;
using ReactiveUI;
using Avalonia.Media;
using GRS.Features.Debug.UIX.Models;
using Studio.Extensions;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace GRS.Features.Debug.UIX.ViewModels;

public class ImageWatchpointDisplayViewModel : ReactiveObject, IWatchpointDisplayViewModel
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
    /// All shader properties that are using this watchpoint
    /// </summary>
    public ObservableCollection<ShaderPropertyViewModel>? ShaderProperties { get; set; }

    /// <summary>
    /// Property collection
    /// </summary>
    public IPropertyViewModel PropertyViewModel { get; set; }

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
    /// Should early depth stencil be enabled?
    /// </summary>
    public bool EarlyDepthStencil
    {
        get => _earlyDepthStencil;
        set => this.RaiseAndSetIfChanged(ref _earlyDepthStencil, value);
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
    /// Render the image in SRGB?
    /// </summary>
    public bool IsSRGB
    {
        get => _isSRGB;
        set => this.RaiseAndSetIfChanged(ref _isSRGB, value);
    }
    
    /// <summary>
    /// Toggle command
    /// </summary>
    public ICommand ToggleColorMaskCommand { get; }

    public ImageWatchpointDisplayViewModel()
    {
        // Values that require reinstrumentation
        this.WhenAnyValue(x => x.Compress, x=> x.EarlyDepthStencil)
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
        ShaderProperties?.ForEach(x => x.EnqueueFirstParentBus());
    }

    /// <summary>
    /// Apply all local watchpoint instrumentation data
    /// </summary>
    /// <param name="config"></param>
    public void ApplyWatchpointConfig(WatchpointConfig config)
    {
        if (_compress)
        {
            config.Flags |= WatchpointFlag.AllowImageFPUNorm8888Compression;
        }

        if (_earlyDepthStencil)
        {
            config.Flags |= WatchpointFlag.EarlyDepthStencil;
        }
    }

    /// <summary>
    /// Internal image
    /// </summary>
    private IImage? _image = null;

    /// <summary>
    /// Internal property
    /// </summary>
    private ShaderPropertyViewModel? _shaderProperty;

    /// <summary>
    /// Internal aspect ratio state
    /// </summary>
    private bool _maintainAspectRatio = true;

    /// <summary>
    /// Internal compress state
    /// </summary>
    private bool _compress = true;

    /// <summary>
    /// Internal ds state
    /// </summary>
    private bool _earlyDepthStencil = false;

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

    /// <summary>
    /// Internal SRGB state
    /// </summary>
    private bool _isSRGB = true;
}
