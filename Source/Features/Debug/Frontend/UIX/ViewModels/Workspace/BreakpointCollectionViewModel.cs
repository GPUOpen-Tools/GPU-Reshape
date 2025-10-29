using System.Collections.ObjectModel;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointCollectionViewModel : BasePropertyViewModel
{
    /// <summary>
    /// All breakpoints within this collection
    /// </summary>
    public ObservableCollection<BreakpointViewModel> Breakpoints { get; } = new();
    
    /// <summary>
    /// Workspace collection property
    /// </summary>
    public required IPropertyViewModel PropertyViewModel { get; set; }

    public BreakpointCollectionViewModel() : base("Collection", PropertyVisibility.Default)
    {
        
    }
}
