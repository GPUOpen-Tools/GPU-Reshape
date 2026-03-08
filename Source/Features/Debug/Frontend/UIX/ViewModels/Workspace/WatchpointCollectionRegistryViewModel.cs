using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using DynamicData;
using Runtime.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Debug.UIX.ViewModels;

public class WatchpointCollectionRegistryViewModel : BasePropertyViewModel
{
    /// <summary>
    /// All collections within this registry
    /// </summary>
    public Dictionary<object, WatchpointCollectionViewModel> ViewModels { get; } = new();

    /// <summary>
    /// All collections, observable
    /// </summary>
    public ObservableCollection<KeyValuePair<object, WatchpointCollectionViewModel>> Collections { get; } = new();
    
    public WatchpointCollectionRegistryViewModel() : base("Registry", PropertyVisibility.Default)
    {
        
    }

    /// <summary>
    /// Find or add a new collection
    /// </summary>
    public WatchpointCollectionViewModel FindOrAdd(object viewModel)
    {
        // Check existing
        if (!ViewModels.TryGetValue(viewModel, out WatchpointCollectionViewModel? value))
        {
            value = new WatchpointCollectionViewModel()
            {
                PropertyViewModel = this.GetWorkspaceCollection()!,
                ViewModel = viewModel,
            };
            
            ViewModels.Add(viewModel, value);
            Collections.Add(KeyValuePair.Create(viewModel, value));
        }

        return value;
    }

    /// <summary>
    /// Remove a watchpoint view model
    /// </summary>
    public void Remove(WatchpointViewModel watchpointViewModel)
    {
        foreach (KeyValuePair<object, WatchpointCollectionViewModel> pair in ViewModels)
        {
            // Remove from physical and virtual
            pair.Value.TextualBindings.RemoveMany(pair.Value.TextualBindings.Where(x => x.WatchpointViewModel == watchpointViewModel));
            pair.Value.SourceBindings.RemoveMany(pair.Value.SourceBindings.Where(x => x.WatchpointViewModel == watchpointViewModel));
            
            // Remove all bound events
            pair.Value.Disposable.Clear();
        }
    }
}
