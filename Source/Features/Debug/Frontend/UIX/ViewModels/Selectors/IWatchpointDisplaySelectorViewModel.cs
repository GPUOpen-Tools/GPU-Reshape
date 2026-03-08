using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels.Selectors;

public interface IWatchpointDisplaySelectorViewModel
{
    /// <summary>
    /// Get the priority of this selector against the format
    /// </summary>
    /// <returns></returns>
    int? GetPriority(DebugWatchpointStreamMessage message);
}
