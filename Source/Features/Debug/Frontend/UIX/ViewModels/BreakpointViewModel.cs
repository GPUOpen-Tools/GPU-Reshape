using System;
using System.Linq;
using System.Windows.Input;
using Avalonia.Threading;
using GRS.Features.Debug.UIX.Models;
using ReactiveUI;
using GRS.Features.Debug.UIX.Settings;
using GRS.Features.Debug.UIX.ViewModels.Processor;
using Message.CLR;
using Studio.Services;
using Studio;
using Studio.Models.Workspace.Listeners;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;
using ShaderViewModel = Studio.ViewModels.Workspace.Properties.Instrumentation.ShaderViewModel;
using Type = Studio.Models.IL.Type;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointViewModel : ReactiveObject, ISourceObjectDetailViewModel
{
    /// <summary>
    /// Currently assigned view model, owned by the archetype
    /// </summary>
    public IBreakpointDisplayViewModel? DisplayViewModel
    {
        get => MonitorRead(() => _displayViewModel);
        set => MonitorWrite(() => this.RaiseAndSetIfChanged(ref _displayViewModel, value));
    }

    /// <summary>
    /// Owning shader content
    /// </summary>
    public required ITextualShaderContentViewModel ShaderContentViewModel { get; set; }
    
    /// <summary>
    /// Source object of this breakpoint
    /// </summary>
    public BreakpointSourceObject TextualSourceObject { get; set; }
    
    /// <summary>
    /// Locating source segment
    /// </summary>
    public ShaderSourceSegment ShaderSourceSegment { get; set; }

    /// <summary>
    /// Currently assigned processor, owned by the archetype
    /// </summary>
    public IBreakpointProcessorViewModel? ProcessorViewModel
    {
        get => MonitorRead(() => _processorViewModel);
        set => MonitorWrite(() => this.RaiseAndSetIfChanged(ref _processorViewModel, value));
    }

    /// <summary>
    /// Currently assigned archetype
    /// </summary>
    public BreakpointDisplayArchetypeViewModel? ArchetypeViewModel
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
    /// Valid archetypes for this breakpoint
    /// </summary>
    public BreakpointDisplayArchetypeViewModel[] Archetypes
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
    /// Shader property
    /// </summary>
    public ShaderViewModel? ShaderProperty { get; set; }

    /// <summary>
    /// The source location of the breakpoint
    /// </summary>
    public SourceBinding? SourceBinding
    {
        get => _sourceBinding;
        set => this.RaiseAndSetIfChanged(ref _sourceBinding, value);
    }

    /// <summary>
    /// Assigned capture mode
    /// </summary>
    public BreakpointCaptureMode CaptureMode { get; set; } = BreakpointCaptureMode.FirstEvent;
    
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
    /// Last time it was requested
    /// </summary>
    public long ProcessThreadLastTimeStamp = 0;

    /// <summary>
    /// Statically allocated UID
    /// </summary>
    public uint UID { get; set; }

    /// <summary>
    /// Current streaming size
    /// </summary>
    public ulong StreamSize { get; set; }

    public BreakpointViewModel()
    {
        StreamSize = GetDefaultSize();

        // Create commands
        OpenInNewCommand = ReactiveCommand.Create(OnOpenInNew);
    }

    private void OnOpenInNew()
    {
        // Open window
        ServiceRegistry.Get<IWindowService>()?.OpenFor(this);
    }

    /// <summary>
    /// Apply the breakpoint configuration
    /// </summary>
    public BreakpointConfig GetBreakpointConfig()
    {
        BreakpointConfig config = new();
        _displayViewModel?.ApplyBreakpointConfig(config);
        return config;
    }

    /// <summary>
    /// Get the default streaming size of a breakpoint
    /// </summary>
    public static uint GetDefaultSize()
    {
        // TODO[dbg]: Ugly, have a standardized unit somewhere
        
        if (ServiceRegistry.Get<ISettingsService>()?.Get<DebugSettingViewModel>() is { } debugSettingViewModel)
        {
            return debugSettingViewModel.DefaultBreakpointMemoryMb * 1000000;
        }

        return 32000000;
    }

    /// <summary>
    /// Get or create the processor that's appropriate for a given stream
    /// </summary>
    /// <param name="message">stream format</param>
    /// <param name="disposable">must be disposed on the UI thread</param>
    /// <returns>null if none appropriate</returns>
    public IBreakpointProcessorViewModel? GetOrCreateProcessor(DebugBreakpointStreamMessage message, out IDisposable? disposable)
    {
        disposable = null;
        
        // To flat
        DebugBreakpointStreamMessage.FlatInfo flat = message.Flat;
        
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
            if (ServiceRegistry.Get<BreakpointDisplayRegistryService>()?.FindOptimalArchetypes(message) is not { Length: > 0 } archetypes || 
                archetypes.First() == _archetypeViewModel)
            {
                disposable = new ActionDisposable(() => Decorate(flat));
                return _processorViewModel;
            }

            // Assume the first
            BreakpointDisplayArchetypeViewModel archetypeViewModel = archetypes.First();
            
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
                    _displayViewModel.ShaderProperty = ShaderProperty;
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

    private void Decorate(DebugBreakpointStreamMessage.FlatInfo flat)
    {
        // Get the typed data
        var order       = (BreakpointDataOrder)flat.dataOrder;
        var compression = (BreakpointCompression)flat.dataCompression;
        
        // Execution information
        Decoration = $"Width:{flat.dataStaticWidth} Height:{flat.dataStaticHeight} Depth:{flat.dataStaticDepth} Compression:{compression} Order:{order}";
    }

    /// <summary>
    /// Lock the user archetype
    /// </summary>
    private void LockUserArchetype(BreakpointDisplayArchetypeViewModel archetypeViewModel)
    {
        // Mark it as locked
        ArchetypeLocked = true;
        
        // Create the processor and display
        _processorViewModel = archetypeViewModel.CreateProcessor();
        _displayViewModel = archetypeViewModel.CreateDisplay();
        _displayViewModel.ShaderProperty = ShaderProperty;
        
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
    private IBreakpointDisplayViewModel? _displayViewModel;

    /// <summary>
    /// Internal source binding
    /// </summary>
    private SourceBinding? _sourceBinding;
    
    /// <summary>
    /// Internal frame rate
    /// </summary>
    private float _frameRate = 0f;

    /// <summary>
    /// Internal processor
    /// </summary>
    private IBreakpointProcessorViewModel? _processorViewModel;
    
    /// <summary>
    /// Internal archetypes
    /// </summary>
    private BreakpointDisplayArchetypeViewModel? _archetypeViewModel;
    
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
    private BreakpointDisplayArchetypeViewModel[] _archetypes = [];

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
}