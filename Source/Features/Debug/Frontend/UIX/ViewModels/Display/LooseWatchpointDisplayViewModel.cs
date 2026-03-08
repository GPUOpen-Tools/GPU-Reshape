using ReactiveUI;
using System.Collections.ObjectModel;
using GRS.Features.Debug.UIX.Models;
using Studio.ViewModels.Workspace.Properties.Instrumentation;
using Message.CLR;
using Studio.Models.IL;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Debug.UIX.ViewModels;

public class LooseWatchpointDisplayViewModel : ReactiveObject, IWatchpointDisplayViewModel
{
    /// <summary>
    /// Flat stream info
    /// </summary>
    public DebugWatchpointStreamMessage.FlatInfo FlatInfo { get; set; }

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
    /// All shader properties that are using this watchpoint
    /// </summary>
    public ObservableCollection<ShaderPropertyViewModel> ShaderProperties { get; set; }

    /// <summary>
    /// Property collection
    /// </summary>
    public IPropertyViewModel PropertyViewModel { get; set; }
    
    /// <summary>
    /// Thin type
    /// </summary>
    public Type Type { get; set; }
    
    /// <summary>
    /// Apply all local watchpoint config
    /// </summary>
    public void ApplyWatchpointConfig(WatchpointConfig config)
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
