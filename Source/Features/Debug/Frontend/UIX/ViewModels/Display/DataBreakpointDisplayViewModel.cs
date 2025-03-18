using ReactiveUI;

namespace GRS.Features.Debug.UIX.ViewModels;

public class DataBreakpointDisplayViewModel : ReactiveObject, IBreakpointDisplayViewModel
{
    /// <summary>
    /// Assocaited raw data
    /// </summary>
    public byte[] RawData { get; set; }
    
    /// <summary>
    /// Dimension counts
    /// </summary>
    public int[] Dimensions;
}
