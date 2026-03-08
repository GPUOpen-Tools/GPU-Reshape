using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive.Disposables;
using System.Reactive.Linq;
using System.Windows.Input;
using Avalonia.Threading;
using GRS.Features.Debug.UIX.Models;
using ReactiveUI;
using GRS.Features.Debug.UIX.Settings;
using GRS.Features.Debug.UIX.ViewModels.Processor;
using Message.CLR;
using Studio.Services;
using Studio;
using Studio.Extensions;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Instrumentation;
using Type = Studio.Models.IL.Type;

namespace GRS.Features.Debug.UIX.ViewModels;

public class WatchpointViewModel : ReactiveObject, ISourceObjectDetailViewModel
{
    /// <summary>
    /// Currently assigned view model, owned by the archetype
    /// </summary>
    public IWatchpointDisplayViewModel? DisplayViewModel
    {
        get => MonitorRead(() => _displayViewModel);
        set => MonitorWrite(() => this.RaiseAndSetIfChanged(ref _displayViewModel, value));
    }
    
    /// <summary>
    /// Currently assigned processor, owned by the archetype
    /// </summary>
    public IWatchpointProcessorViewModel? ProcessorViewModel
    {
        get => MonitorRead(() => _processorViewModel);
        set => MonitorWrite(() => this.RaiseAndSetIfChanged(ref _processorViewModel, value));
    }

    /// <summary>
    /// Currently assigned archetype
    /// </summary>
    public WatchpointDisplayArchetypeViewModel? ArchetypeViewModel
    {
        get => MonitorRead(() => _archetypeViewModel);
        set => MonitorWrite(() =>
        {
            if (value != null)
            {
                LockUserArchetype(value);
            }
            
            this.RaiseAndSetIfChanged(ref _archetypeViewModel, value);
        });
    }

    /// <summary>
    /// Valid archetypes for this watchpoint
    /// </summary>
    public WatchpointDisplayArchetypeViewModel[] Archetypes
    {
        get => _archetypes;
        private set => this.RaiseAndSetIfChanged(ref _archetypes, value);
    }

    /// <summary>
    /// is the archetype locked?
    /// </summary>
    public bool ArchetypeLocked
    {
        get => _archetypeLocked;
        set => this.RaiseAndSetIfChanged(ref _archetypeLocked, value);
    }

    /// <summary>
    /// Is the collection paused?
    /// </summary>
    public bool Paused
    {
        get => _paused;
        set => this.RaiseAndSetIfChanged(ref _paused, value);
    }

    /// <summary>
    /// Optional status message
    /// </summary>
    public string StatusMessage
    {
        get => _statusMessage;
        set => this.RaiseAndSetIfChanged(ref _statusMessage, value);
    }

    /// <summary>
    /// Any status to be displayed
    /// </summary>
    public bool HasStatusMessage
    {
        get => _hasStatusMessage;
        set => this.RaiseAndSetIfChanged(ref _hasStatusMessage, value);
    }

    /// <summary>
    /// Open in new window command
    /// </summary>
    public ICommand OpenInNewCommand { get; private set; }
    
    /// <summary>
    /// Intermediate tiny type
    /// </summary>
    public Type TinyType { get; set; }

    /// <summary>
    /// All shader properties that are using this watchpoint
    /// </summary>
    public ObservableCollection<ShaderPropertyViewModel> ShaderProperties { get; } = new();

    /// <summary>
    /// All shader properties that are using this watchpoint
    /// </summary>
    public IPropertyViewModel PropertyViewModel { get; set; }

    /// <summary>
    /// Assigned capture mode
    /// </summary>
    public WatchpointCaptureMode CaptureMode
    {
        get => _captureMode;
        set => this.RaiseAndSetIfChanged(ref _captureMode, value);
    }

    /// <summary>
    /// All allowed capture modes
    /// </summary>
    public Array CaptureModeTypes { get; } = Enum.GetValues<WatchpointCaptureMode>();

    /// <summary>
    /// All debug variables
    /// </summary>
    public ObservableCollection<WatchpointDebugVariable> DebugVariables { get; } = new();

    /// <summary>
    /// Selected debug value
    /// </summary>
    public WatchpointDebugValue? SelectedDebugValue
    {
        get => _selectedDebugValue;
        set => this.RaiseAndSetIfChanged(ref _selectedDebugValue, value);
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
    /// Decorated string
    /// </summary>
    public string Decoration
    {
        get => _decoration;
        set => this.RaiseAndSetIfChanged(ref _decoration, value);
    }
    
    /// <summary>
    /// Optional marker
    /// </summary>
    public string Marker
    {
        get => _marker;
        set => this.RaiseAndSetIfChanged(ref _marker, value);
    }

    /// <summary>
    /// Last time it was requested
    /// </summary>
    public long ProcessThreadLastTimeStamp = 0;

    /// <summary>
    /// Statically allocated UID
    /// </summary>
    public uint UID { get; set; } = uint.MaxValue;

    /// <summary>
    /// Current streaming size
    /// </summary>
    public ulong StreamSize { get; set; }
    
    /// <summary>
    /// Shared disposable
    /// </summary>
    public CompositeDisposable Disposable { get; } = new();

    public WatchpointViewModel()
    {
        StreamSize = GetDefaultSize();

        // Create commands
        OpenInNewCommand = ReactiveCommand.Create(OnOpenInNew);
        
        // Update shaders on marker changes
        this.WhenAnyValue(x => x.Marker)
            .Skip(1)
            .Throttle(TimeSpan.FromMilliseconds(500))
            .Subscribe(_ => EnqueueAllShaderParentBus());
        
        // Update shaders on variable changes
        this.WhenAnyValue(x => x.SelectedDebugValue)
            .WhereNotNull()
            .Subscribe(_ => EnqueueAllShaderParentBus());
    }

    /// <summary>
    /// Enqueue all parent buses of bound shaders
    /// </summary>
    public void EnqueueAllShaderParentBus()
    {
        ShaderProperties.ForEach(x => x.EnqueueFirstParentBus());
    }

    private void OnOpenInNew()
    {
        // Open window
        ServiceRegistry.Get<IWindowService>()?.OpenFor(this);
    }

    /// <summary>
    /// Apply the watchpoint configuration
    /// </summary>
    public WatchpointConfig GetWatchpointConfig()
    {
        WatchpointConfig config = new();
        _displayViewModel?.ApplyWatchpointConfig(config);
        return config;
    }

    /// <summary>
    /// Get the default streaming size of a watchpoint
    /// </summary>
    public static uint GetDefaultSize()
    {
        // TODO[dbg]: Ugly, have a standardized unit somewhere
        
        if (ServiceRegistry.Get<ISettingsService>()?.Get<DebugSettingViewModel>() is { } debugSettingViewModel)
        {
            return debugSettingViewModel.DefaultWatchpointMemoryMb * 1000000;
        }

        return 32000000;
    }

    /// <summary>
    /// Get or create the processor that's appropriate for a given stream
    /// </summary>
    /// <param name="message">stream format</param>
    /// <param name="disposable">must be disposed on the UI thread</param>
    /// <returns>null if none appropriate</returns>
    public IWatchpointProcessorViewModel? GetOrCreateProcessor(DebugWatchpointStreamMessage message, out IDisposable? disposable)
    {
        disposable = null;
        
        // To flat
        DebugWatchpointStreamMessage.FlatInfo flat = message.Flat;
        
        lock (_monitor)
        {
            // May be locked
            if (_archetypeLocked && _processorViewModel != null)
            {
                disposable = new ActionDisposable(() =>
                {
                    Dispatcher.UIThread.VerifyAccess();
                    Decorate(flat);
                });

                // Just assume the current
                return _processorViewModel;
            }

            // Always try to find a new archetype that's a better fit
            // The underlying data format may change depending on what's happening
            var registryService = ServiceRegistry.Get<WatchpointDisplayRegistryService>();
            if (registryService?.FindOptimalArchetypes(message) is not { Length: > 0 } archetypes || 
                archetypes.First() == _archetypeViewModel)
            {
                disposable = new ActionDisposable(() =>
                {
                    // Special case, if the ordering failed entirely, fallback on loose streaming
                    if (Archetypes.Length == 0)
                    {
                        CaptureMode = WatchpointCaptureMode.AllEvents;
                    }
                    
                    Decorate(flat);
                });
                return _processorViewModel;
            }

            // Assume the first
            WatchpointDisplayArchetypeViewModel archetypeViewModel = archetypes.First();
            
            // Create the processor on the calling thread
            var processor = archetypeViewModel.CreateProcessor();
                
            // Create UI disposable
            disposable = new ActionDisposable(() =>
            {
                Dispatcher.UIThread.VerifyAccess();

                // Assign valid archetypes, doesn't have to be atomic
                Archetypes = archetypes;

                // Finalize objects
                lock (_monitor)
                {
                    _processorViewModel = processor;
                    _archetypeViewModel = archetypeViewModel;
                    
                    // Create view model
                    _displayViewModel = archetypeViewModel.CreateDisplay();
                    _displayViewModel.ShaderProperties = ShaderProperties;
                    _displayViewModel.PropertyViewModel = PropertyViewModel;
                }
                
                // Decorate the flat
                Decorate(flat);
                
                // Raise, this doesn't have to be atomic
                this.RaisePropertyChanged(nameof(DisplayViewModel));
                this.RaisePropertyChanged(nameof(ProcessorViewModel));
                this.RaisePropertyChanged(nameof(ArchetypeViewModel));
            });

            // OK
            return processor;
        }
    }

    private void Decorate(DebugWatchpointStreamMessage.FlatInfo flat)
    {
        // Get the typed data
        var order       = (WatchpointDataOrder)flat.dataOrder;
        var compression = (WatchpointCompression)flat.dataCompression;
        
        // Execution information
        Decoration = $"Width:{flat.dataStaticWidth} Height:{flat.dataStaticHeight} Depth:{flat.dataStaticDepth} Compression:{compression} Order:{order}";
    }

    /// <summary>
    /// Lock the user archetype
    /// </summary>
    private void LockUserArchetype(WatchpointDisplayArchetypeViewModel archetypeViewModel)
    {
        // Mark it as locked
        ArchetypeLocked = true;
        
        // Create the processor and display
        _processorViewModel = archetypeViewModel.CreateProcessor();
        _displayViewModel = archetypeViewModel.CreateDisplay();
        _displayViewModel.ShaderProperties = ShaderProperties;
        _displayViewModel.PropertyViewModel = PropertyViewModel;
        
        // Raise
        this.RaisePropertyChanged(nameof(DisplayViewModel));
        this.RaisePropertyChanged(nameof(_processorViewModel));
        this.RaisePropertyChanged(nameof(_archetypeViewModel));
    }

    /// <summary>
    /// Helper for threaded reads
    /// </summary>
    private T MonitorRead<T>(Func<T> func)
    {
        lock (_monitor)
        {
            return func();
        }
    }

    /// <summary>
    /// Helper for threaded writes
    /// </summary>
    private void MonitorWrite(Action func)
    {
        Dispatcher.UIThread.VerifyAccess();

        lock (_monitor)
        {
            func();
        }
    }

    /// <summary>
    /// Internal view model
    /// </summary>
    private IWatchpointDisplayViewModel? _displayViewModel;

    /// <summary>
    /// Internal frame rate
    /// </summary>
    private float _frameRate = 0f;

    /// <summary>
    /// Internal processor
    /// </summary>
    private IWatchpointProcessorViewModel? _processorViewModel;
    
    /// <summary>
    /// Internal archetypes
    /// </summary>
    private WatchpointDisplayArchetypeViewModel? _archetypeViewModel;
    
    /// <summary>
    /// Internal lock state
    /// </summary>
    private bool _archetypeLocked = false;
    
    /// <summary>
    /// Shared monitor for archetype atomicity
    /// </summary>
    private object _monitor = new();

    /// <summary>
    /// Internal decoration
    /// </summary>
    private string _decoration;

    /// <summary>
    /// Internal archetypes
    /// </summary>
    private WatchpointDisplayArchetypeViewModel[] _archetypes = [];

    /// <summary>
    /// Internal pause state
    /// </summary>
    private bool _paused = false;

    /// <summary>
    /// Internal status state
    /// </summary>
    private string _statusMessage = string.Empty;

    /// <summary>
    /// Internal status state
    /// </summary>
    private bool _hasStatusMessage;

    /// <summary>
    /// Internal capture mode
    /// </summary>
    private WatchpointCaptureMode _captureMode = WatchpointCaptureMode.FirstEvent;

    /// <summary>
    /// Internal marker
    /// </summary>
    private string _marker;

    /// <summary>
    /// Internal value
    /// </summary>
    private WatchpointDebugValue? _selectedDebugValue;
}