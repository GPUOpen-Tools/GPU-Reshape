using ReactiveUI;
using GRS.Features.Debug.UIX.Models;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace GRS.Features.Debug.UIX.ViewModels;

public class StructuredBreakpointDisplayViewModel : ReactiveObject, IBreakpointDisplayViewModel
{
    /// <summary>
    /// Associated raw data
    /// </summary>
    public byte[] RawData { get; set; }
    
    /// <summary>
    /// Dimension counts
    /// </summary>
    public int[] Dimensions;

    /// <summary>
    /// Shader property
    /// </summary>
    public ShaderViewModel? ShaderProperty { get; set; }
    
    /// <summary>
    /// Apply all local breakpoint config
    /// </summary>
    public void ApplyBreakpointConfig(BreakpointConfig config)
    {
        
    }
}
