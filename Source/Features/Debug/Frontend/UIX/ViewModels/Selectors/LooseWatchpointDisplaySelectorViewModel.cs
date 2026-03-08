using GRS.Features.Debug.UIX.Models;
using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels.Selectors;

public class LooseWatchpointDisplaySelectorViewModel : IWatchpointDisplaySelectorViewModel
{
    public int? GetPriority(DebugWatchpointStreamMessage message)
    {
        // Get the typed data
        var order = (WatchpointDataOrder)message.dataOrder;

        // If the data is loose, this is best
        if (order == WatchpointDataOrder.Loose)
        {
            return WatchpointDisplaySelectorPriority.Optimal;
        }

        // No can do
        return WatchpointDisplaySelectorPriority.Unsupported;
    }
}
