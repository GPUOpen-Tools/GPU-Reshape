using System.Collections.ObjectModel;
using System.Reactive.Disposables;
using GRS.Features.Debug.UIX.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Debug.UIX.ViewModels;

public class WatchpointViewModelSourceBinding
{
    /// <summary>
    /// Watchpoint of the binding
    /// </summary>
    public required WatchpointViewModel WatchpointViewModel { get; set; }
    
    /// <summary>
    /// Location it's bound to
    /// </summary>
    public required SourceBinding Source { get; set; }
}

public class WatchpointViewModelTextualBinding
{
    /// <summary>
    /// Watchpoint of the binding
    /// </summary>
    public required WatchpointViewModel WatchpointViewModel { get; set; }
    
    /// <summary>
    /// The line the watchpoint is bound to
    /// </summary>
    public int LineBase0 { get; set; }
}

public class WatchpointCollectionViewModel : BasePropertyViewModel
{
    /// <summary>
    /// All watchpoint source (physical) bindings within this collection
    /// </summary>
    public ObservableCollection<WatchpointViewModelSourceBinding> SourceBindings { get; } = new();
    
    /// <summary>
    /// All watchpoints textual (virtual) within this collection
    /// </summary>
    public ObservableCollection<WatchpointViewModelTextualBinding> TextualBindings { get; } = new();
    
    /// <summary>
    /// Workspace collection property
    /// </summary>
    public required IPropertyViewModel PropertyViewModel { get; set; }

    /// <summary>
    /// The assigned view model
    /// </summary>
    public required object ViewModel { get; set; }
    
    /// <summary>
    /// Shared disposable
    /// </summary>
    public CompositeDisposable Disposable { get; } = new();

    public WatchpointCollectionViewModel() : base("Collection", PropertyVisibility.Default)
    {
        
    }

    /// <summary>
    /// Register a watchpoint to this collection and its providers
    /// </summary>
    public void Register(WatchpointViewModelTextualBinding binding)
    {
        TextualBindings.Add(binding);
        
        // Remove on disposed
        binding.WatchpointViewModel.Disposable.Add(System.Reactive.Disposables.Disposable.Create(() =>
        {
            TextualBindings.Remove(binding);
        }));
        
        // Register against registry
        PropertyViewModel.GetService<WatchpointRegistryService>()?.Register(binding.WatchpointViewModel);
    }
}
