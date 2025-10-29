using System.Collections.Generic;
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
}
