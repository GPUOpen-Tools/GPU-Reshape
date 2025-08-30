using GRS.Features.Debug.UIX.Models;
using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels.Selectors;

public class LooseBreakpointDisplaySelectorViewModel : IBreakpointDisplaySelectorViewModel
{
    public int? GetPriority(DebugBreakpointStreamMessage message)
    {
        // Get the typed data
        var order = (BreakpointDataOrder)message.dataOrder;

        // If the data is loose, this is best
        if (order == BreakpointDataOrder.Loose)
        {
            return BreakpointDisplaySelectorPriority.Optimal;
        }

        // No can do
        return BreakpointDisplaySelectorPriority.Unsupported;
    }
}
