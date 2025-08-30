using ReactiveUI;
using System.Collections.ObjectModel;
using GRS.Features.Debug.UIX.Models;
using Studio.ViewModels.Workspace.Properties.Instrumentation;
using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels;

public class LooseBreakpointDisplayViewModel : ReactiveObject, IBreakpointDisplayViewModel
{
    /// <summary>
    /// Flat stream info
    /// </summary>
    public DebugBreakpointStreamMessage.FlatInfo FlatInfo { get; set; }

    /// <summary>
    /// All loose items, virtualized container
    /// </summary>
    public LooseVirtualObservableCollection? Items
    {
        get => _items;
        set => this.RaiseAndSetIfChanged(ref _items, value);
    }

    /// <summary>
    /// Associated raw data
    /// </summary>
    public uint[] DWords
    {
        get => _dwords;
        set => this.RaiseAndSetIfChanged(ref _dwords, value);
    }

    /// <summary>
    /// Shader property
    /// </summary>
    public ShaderViewModel? ShaderProperty { get; set; }
    
    /// <summary>
    /// Apply all local breakpoint config
    /// </summary>
    public void ApplyBreakpointConfig(BreakpointConfig config)
    {
        
    }
    
    /// <summary>
    /// Internal data
    /// </summary>
    private uint[] _dwords;

    /// <summary>
    /// Internal container
    /// </summary>
    private LooseVirtualObservableCollection? _items;
}
