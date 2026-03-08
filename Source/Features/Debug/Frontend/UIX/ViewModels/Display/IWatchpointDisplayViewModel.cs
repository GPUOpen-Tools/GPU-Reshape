using System.Collections.ObjectModel;
using GRS.Features.Debug.UIX.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace GRS.Features.Debug.UIX.ViewModels;

public interface IWatchpointDisplayViewModel : IWatchpointInstrumentationObject
{
    /// <summary>
    /// All shader properties that are using this watchpoint
    /// </summary>
    public ObservableCollection<ShaderPropertyViewModel> ShaderProperties { get; set; }
    
    /// <summary>
    /// Collection property
    /// </summary>
    public IPropertyViewModel PropertyViewModel { get; set; }
}
