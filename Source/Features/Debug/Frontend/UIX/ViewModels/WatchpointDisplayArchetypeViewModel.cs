using System;
using System.Collections.ObjectModel;
using GRS.Features.Debug.UIX.ViewModels.Processor;
using GRS.Features.Debug.UIX.ViewModels.Selectors;

namespace GRS.Features.Debug.UIX.ViewModels;

public class WatchpointDisplayArchetypeViewModel
{
    /// <summary>
    /// Name of this archetype
    /// </summary>
    public string Name { set; get; }

    /// <summary>
    /// The display archetype
    /// </summary>
    public Type Archetype { get; set; }

    /// <summary>
    /// The selector view model
    /// Assigns a given priority for the archetype
    /// </summary>
    public IWatchpointDisplaySelectorViewModel Selector { get; set; }
    
    /// <summary>
    /// All processor modes
    /// </summary>
    public ObservableCollection<Type> ProcessorArchetypes { get; set; } = new();

    /// <summary>
    /// Create the display view model
    /// </summary>
    public IWatchpointDisplayViewModel CreateDisplay()
    {
        // TODO[dbg]: Temp code
        return (IWatchpointDisplayViewModel)Activator.CreateInstance(Archetype)!;
    }

    /// <summary>
    /// Create the processor view model
    /// </summary>
    public IWatchpointProcessorViewModel? CreateProcessor()
    {
        // TODO[dbg]: Temp code
        return (IWatchpointProcessorViewModel)Activator.CreateInstance(ProcessorArchetypes[0])!;
    }
}
