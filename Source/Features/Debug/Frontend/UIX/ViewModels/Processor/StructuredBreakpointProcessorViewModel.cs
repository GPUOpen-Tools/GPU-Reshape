using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels.Processor;

public class StructuredBreakpointProcessorViewModel : IBreakpointProcessorViewModel
{
    /// <summary>
    /// Process a breakpoint stream, this happens on a separate thread
    /// Must not interact with the UI thread
    /// </summary>
    /// <returns>optional payload data</returns>
    public object? Process(DebugBreakpointStreamMessage message)
    {
        return null;
    }

    /// <summary>
    /// Install the payload on the UI thread
    /// </summary>
    public void Install(IBreakpointDisplayViewModel displayViewModel, object payload)
    {
    }
}
