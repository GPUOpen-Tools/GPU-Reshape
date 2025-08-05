using GRS.Features.Debug.UIX.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace GRS.Features.Debug.UIX.ViewModels;

public interface IBreakpointDisplayViewModel : IBreakpointInstrumentationObject
{
    /// <summary>
    /// Shader property
    /// </summary>
    public ShaderViewModel? ShaderProperty { get; set; }
}
