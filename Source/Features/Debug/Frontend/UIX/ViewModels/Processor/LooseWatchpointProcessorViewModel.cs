using System;
using Message.CLR;
using Type = Studio.Models.IL.Type;

namespace GRS.Features.Debug.UIX.ViewModels.Processor;

public class LooseWatchpointProcessorViewModel : IWatchpointProcessorViewModel
{
    /// <summary>
    /// Process a watchpoint stream, this happens on a separate thread
    /// Must not interact with the UI thread
    /// </summary>
    /// <returns>optional payload data</returns>
    public unsafe object? Process(WatchpointViewModel watchpointViewModel, DebugWatchpointStreamMessage message)
    {
        var data = new uint[message.data.Count / sizeof(uint)];
        
        // Lifetime is owned by the stream, so copy it over
        // Really, we shouldn't have to do this, the processing should happen entirely async
        fixed (uint* dest = data)
        {
            Buffer.MemoryCopy(message.data.GetDataStart(), dest, (int)message.data.Count, (int)message.data.Count);
        }
        
        return new Payload()
        {
            Flat = message.Flat,
            Type = watchpointViewModel.TinyType,
            Data = data
        };
    }

    /// <summary>
    /// Install the payload on the UI thread
    /// </summary>
    public void Install(IWatchpointDisplayViewModel displayViewModel, object payload)
    {
        var typed = (Payload)payload;
        
        // Assign contents
        if (displayViewModel is LooseWatchpointDisplayViewModel looseDisplayViewModel)
        {
            looseDisplayViewModel.FlatInfo = typed.Flat;
            looseDisplayViewModel.DWords = typed.Data;
            looseDisplayViewModel.Type = typed.Type;
            looseDisplayViewModel.Items = new LooseVirtualObservableCollection()
            {
                WatchpointDisplayViewModel = looseDisplayViewModel
            };
        }
    }

    private struct Payload
    {
        /// <summary>
        /// Flat streaming info
        /// </summary>
        public DebugWatchpointStreamMessage.FlatInfo Flat;

        /// <summary>
        /// Tiny type
        /// </summary>
        public Type Type;
        
        /// <summary>
        /// Owned data
        /// </summary>
        public uint[] Data;
    }
}
