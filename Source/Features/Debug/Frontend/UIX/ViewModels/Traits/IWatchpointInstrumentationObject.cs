using GRS.Features.Debug.UIX.Models;

namespace GRS.Features.Debug.UIX.ViewModels.Traits;

public interface IWatchpointInstrumentationObject
{
    /// <summary>
    /// Apply the watchpoint configuration
    /// </summary>
    /// <param name="config">config state</param>
    public void ApplyWatchpointConfig(WatchpointConfig config);
}
