using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels.Selectors;

public interface IBreakpointDisplaySelectorViewModel
{
    /// <summary>
    /// Get the priority of this selector against the format
    /// </summary>
    /// <returns></returns>
    int? GetPriority(DebugBreakpointStreamMessage message);
}
