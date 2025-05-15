using System;
using System.Collections.ObjectModel;
using GRS.Features.Debug.UIX.ViewModels.Processor;
using GRS.Features.Debug.UIX.ViewModels.Selectors;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointDisplayArchetypeViewModel
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
    public IBreakpointDisplaySelectorViewModel Selector { get; set; }
    
    /// <summary>
    /// All processor modes
    /// </summary>
    public ObservableCollection<Type> ProcessorArchetypes { get; set; } = new();

    /// <summary>
    /// Create the display view model
    /// </summary>
    public IBreakpointDisplayViewModel CreateDisplay()
    {
        // TODO[dbg]: Temp code
        return (IBreakpointDisplayViewModel)Activator.CreateInstance(Archetype)!;
    }

    /// <summary>
    /// Create the processor view model
    /// </summary>
    public IBreakpointProcessorViewModel? CreateProcessor()
    {
        // TODO[dbg]: Temp code
        return (IBreakpointProcessorViewModel)Activator.CreateInstance(ProcessorArchetypes[0])!;
    }
}
