namespace GRS.Features.Debug.UIX.Models;

public enum BreakpointValueReconstructionState
{
    /// <summary>
    /// No state can be reconstructed
    /// </summary>
    None,
    
    /// <summary>
    /// Some state can be reconstructed
    /// </summary>
    Partial,
    
    /// <summary>
    /// The full state can be reconstructed
    /// </summary>
    Full
}
