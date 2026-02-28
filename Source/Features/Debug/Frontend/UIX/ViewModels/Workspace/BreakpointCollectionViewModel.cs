using System.Collections.ObjectModel;
using System.Reactive.Disposables;
using GRS.Features.Debug.UIX.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointViewModelSourceBinding
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

public class BreakpointViewModelTextualBinding
{
    /// <summary>
    /// Breakpoint of the binding
    /// </summary>
    public required BreakpointViewModel BreakpointViewModel { get; set; }
    
    /// <summary>
    /// The line the breakpoint is bound to
    /// </summary>
    public int LineBase0 { get; set; }
}

public class BreakpointCollectionViewModel : BasePropertyViewModel
{
    /// <summary>
    /// All breakpoint source (physical) bindings within this collection
    /// </summary>
    public ObservableCollection<BreakpointViewModelSourceBinding> SourceBindings { get; } = new();
    
    /// <summary>
    /// All breakpoints textual (virtual) within this collection
    /// </summary>
    public ObservableCollection<BreakpointViewModelTextualBinding> TextualBindings { get; } = new();
    
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

    public BreakpointCollectionViewModel() : base("Collection", PropertyVisibility.Default)
    {
        
    }

    /// <summary>
    /// Register a breakpoint to this collection and its providers
    /// </summary>
    public void Register(BreakpointViewModelTextualBinding binding)
    {
        TextualBindings.Add(binding);
        
        // Remove on disposed
        binding.BreakpointViewModel.Disposable.Add(System.Reactive.Disposables.Disposable.Create(() =>
        {
            TextualBindings.Remove(binding);
        }));
        
        // Register against registry
        PropertyViewModel.GetService<BreakpointRegistryService>()?.Register(binding.BreakpointViewModel);
    }
}
