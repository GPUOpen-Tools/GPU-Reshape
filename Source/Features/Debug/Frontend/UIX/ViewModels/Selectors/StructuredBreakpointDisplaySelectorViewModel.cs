using GRS.Features.Debug.UIX.Models;
using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels.Selectors;

public class StructuredBreakpointDisplaySelectorViewModel : IBreakpointDisplaySelectorViewModel
{
    public int? GetPriority(DebugBreakpointStreamMessage message)
    {
        // It's really always supported
        return BreakpointDisplaySelectorPriority.Supported;
    }
}