using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using DynamicData;
using Runtime.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointCollectionRegistryViewModel : BasePropertyViewModel
{
    /// <summary>
    /// All collections within this registry
    /// </summary>
    public Dictionary<object, BreakpointCollectionViewModel> ViewModels { get; } = new();

    /// <summary>
    /// All collections, observable
    /// </summary>
    public ObservableCollection<KeyValuePair<object, BreakpointCollectionViewModel>> Collections { get; } = new();
    
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
            Collections.Add(KeyValuePair.Create(viewModel, value));
        }

        return value;
    }

    /// <summary>
    /// Remove a breakpoint view model
    /// </summary>
    public void Remove(BreakpointViewModel breakpointViewModel)
    {
        foreach (KeyValuePair<object, BreakpointCollectionViewModel> pair in ViewModels)
        {
            pair.Value.Bindings.RemoveMany(pair.Value.Bindings.Where(x => x.BreakpointViewModel == breakpointViewModel));
            
            // Remove all bound events
            pair.Value.Disposable.Clear();
        }
    }
}
