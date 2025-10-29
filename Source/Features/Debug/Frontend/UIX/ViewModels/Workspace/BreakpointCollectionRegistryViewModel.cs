using System.Collections.Generic;
using Runtime.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointCollectionRegistryViewModel : BasePropertyViewModel
{
    /// <summary>
    /// All collections within this registry
    /// </summary>
    public Dictionary<object, BreakpointCollectionViewModel> ViewModels { get; } = new();
    
    public BreakpointCollectionRegistryViewModel() : base("Registry", PropertyVisibility.Default)
    {
        
    }

    /// <summary>
    /// Find or add a new collection
    /// </summary>
    public BreakpointCollectionViewModel FindOrAdd(object viewModel)
    {
        // Check existing
        if (!ViewModels.TryGetValue(viewModel, out BreakpointCollectionViewModel? value))
        {
            value = new BreakpointCollectionViewModel()
            {
                PropertyViewModel = this.GetWorkspaceCollection()!
            };
            
            ViewModels.Add(viewModel, value);
        }

        return value;
    }

    /// <summary>
    /// Remove a breakpoint view model
    /// </summary>
    public void Remove(BreakpointViewModel breakpointViewModel)
    {
        foreach (BreakpointCollectionViewModel collectionViewModel in ViewModels.Values)
        {
            collectionViewModel.Breakpoints.Remove(breakpointViewModel);
        }
    }
}
