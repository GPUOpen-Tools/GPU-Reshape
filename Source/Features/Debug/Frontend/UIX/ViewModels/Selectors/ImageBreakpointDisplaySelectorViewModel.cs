using GRS.Features.Debug.UIX.Models;
using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels.Selectors;

public class ImageBreakpointDisplaySelectorViewModel : IBreakpointDisplaySelectorViewModel
{
    public int? GetPriority(DebugBreakpointStreamMessage message)
    {
        // Get the typed data
        var order       = (BreakpointDataOrder)message.dataOrder;
        var compression = (BreakpointCompression)message.dataOrder;

        // Dynamic data isn't supported yet
        if (order != BreakpointDataOrder.Static)
        {
            return BreakpointDisplaySelectorPriority.Unsupported;
        }

        // Volumetric data isn't supported yet
        if (message.dataStaticDepth > 1)
        {
            return BreakpointDisplaySelectorPriority.Unsupported;
        }

        // If compressed, optimal
        if (compression == BreakpointCompression.FPUNorm8888)
        {
            return BreakpointDisplaySelectorPriority.Optimal;
        }
        
        // Otherwise, the preferred display mode
        return BreakpointDisplaySelectorPriority.Preferred;
    }
}