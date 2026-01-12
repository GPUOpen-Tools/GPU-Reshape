using System.Collections.ObjectModel;
using System.Reactive.Disposables;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointViewModelBinding
{
    /// <summary>
    /// Breakpoint of the binding
    /// </summary>
    public required BreakpointViewModel BreakpointViewModel { get; set; }
    
    /// <summary>
    /// Location it's bound to
    /// </summary>
    public required SourceBinding Source { get; set; }
}

public class BreakpointCollectionViewModel : BasePropertyViewModel
{
    /// <summary>
    /// All breakpoint bindings within this collection
    /// </summary>
    public ObservableCollection<BreakpointViewModelBinding> Bindings { get; } = new();
    
    /// <summary>
    /// Workspace collection property
    /// </summary>
    public required IPropertyViewModel PropertyViewModel { get; set; }
    
    /// <summary>
    /// Shared disposable
    /// </summary>
    public CompositeDisposable Disposable { get; } = new();

    public BreakpointCollectionViewModel() : base("Collection", PropertyVisibility.Default)
    {
        
    }
}
