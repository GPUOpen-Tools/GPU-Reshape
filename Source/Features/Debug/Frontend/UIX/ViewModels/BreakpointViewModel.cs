using System;
using Avalonia.Threading;
using GRS.Features.Debug.UIX.Models;
using ReactiveUI;
using GRS.Features.Debug.UIX.Settings;
using GRS.Features.Debug.UIX.ViewModels.Processor;
using Message.CLR;
using Studio.ViewModels.Workspace.Properties.Instrumentation;
using Studio.Services;
using Studio;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointViewModel : ReactiveObject
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
        set => MonitorWrite(() => this.RaiseAndSetIfChanged(ref _archetypeViewModel, value));
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
    public long LastTimeStamp = 0;

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
        
        lock (_monitor)
        {
            // May be locked
            if (_archetypeLocked)
            {
                return _processorViewModel;
            }

            // To flat
            DebugBreakpointStreamMessage.FlatInfo flat = message.Flat;

            // Always try to find a new archetype that's a better fit
            // The underlying data format may change depending on what's happening
            if (ServiceRegistry.Get<BreakpointDisplayRegistryService>()?.FindOptimalArchetype(message) is not { } archetypeViewModel || archetypeViewModel == _archetypeViewModel)
            {
                disposable = new ActionDisposable(() => Decorate(flat));
                return _processorViewModel;
            }
            
            // Create the processor on the calling thread
            var processor = archetypeViewModel.CreateProcessor();
                
            // Create UI disposable
            disposable = new ActionDisposable(() =>
            {
                Dispatcher.UIThread.VerifyAccess();

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
                this.RaisePropertyChanged(nameof(_processorViewModel));
                this.RaisePropertyChanged(nameof(_archetypeViewModel));
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
}