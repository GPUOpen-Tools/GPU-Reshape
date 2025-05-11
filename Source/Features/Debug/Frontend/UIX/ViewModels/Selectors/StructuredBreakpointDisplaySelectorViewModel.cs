using GRS.Features.Debug.UIX.Models;
using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels.Selectors;

public class StructuredBreakpointDisplaySelectorViewModel : IBreakpointDisplaySelectorViewModel
{
    public int? GetPriority(DebugBreakpointStreamMessage message)
    {
        // Get the typed data
        var order = (BreakpointDataOrder)message.dataOrder;

        // If the data is dynamic, this is best
        if (order == BreakpointDataOrder.Dynamic)
        {
            return BreakpointDisplaySelectorPriority.Optimal;
        }

        // It's really always supported
        return BreakpointDisplaySelectorPriority.Supported;
    }
}