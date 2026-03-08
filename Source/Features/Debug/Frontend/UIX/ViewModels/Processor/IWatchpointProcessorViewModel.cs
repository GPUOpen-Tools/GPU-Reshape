using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels.Processor;

public interface IWatchpointProcessorViewModel
{
    /// <summary>
    /// Process a watchpoint stream, this happens on a separate thread
    /// Must not interact with the UI thread
    /// </summary>
    /// <returns>optional payload data</returns>
    object? Process(WatchpointViewModel watchpointViewModel, DebugWatchpointStreamMessage message);

    /// <summary>
    /// Install the payload on the UI thread
    /// </summary>
    void Install(IWatchpointDisplayViewModel displayViewModel, object payload);
}
