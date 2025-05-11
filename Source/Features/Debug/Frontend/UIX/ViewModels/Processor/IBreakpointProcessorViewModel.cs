using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels.Processor;

public interface IBreakpointProcessorViewModel
{
    /// <summary>
    /// Process a breakpoint stream, this happens on a separate thread
    /// Must not interact with the UI thread
    /// </summary>
    /// <returns>optional payload data</returns>
    object? Process(DebugBreakpointStreamMessage message);

    /// <summary>
    /// Install the payload on the UI thread
    /// </summary>
    void Install(IBreakpointDisplayViewModel displayViewModel, object payload);
}
