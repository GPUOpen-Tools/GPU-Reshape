using GRS.Features.Debug.UIX.Models;

namespace GRS.Features.Debug.UIX.ViewModels.Traits;

public interface IBreakpointInstrumentationObject
{
    /// <summary>
    /// Apply the breakpoint configuration
    /// </summary>
    /// <param name="config">config state</param>
    public void ApplyBreakpointConfig(BreakpointConfig config);
}
