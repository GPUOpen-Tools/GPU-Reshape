using System.Linq;
using System.Collections.ObjectModel;
using AvaloniaEdit.Utils;
using GRS.Features.Debug.UIX.ViewModels.Processor;
using GRS.Features.Debug.UIX.ViewModels.Selectors;
using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels;

public class BreakpointDisplayRegistryService
{
    /// <summary>
    /// All archetypes
    /// </summary>
    public ObservableCollection<BreakpointDisplayArchetypeViewModel> Archetypes { get; set; } = new();

    public BreakpointDisplayRegistryService()
    {
        // Setup the standard archetypes
        Archetypes.AddRange([
            // Standard image view
            new BreakpointDisplayArchetypeViewModel
            {
                Name = "Image",
                Archetype = typeof(ImageBreakpointDisplayViewModel),
                Selector = new ImageBreakpointDisplaySelectorViewModel(),
                ProcessorArchetypes = 
                [
                    typeof(ImageBreakpointProcessorViewModel)
                ]
            },
            
            // Structured display
            new BreakpointDisplayArchetypeViewModel
            {
                Name = "Structured",
                Archetype = typeof(StructuredBreakpointDisplayViewModel),
                Selector = new StructuredBreakpointDisplaySelectorViewModel(),
                ProcessorArchetypes = 
                [
                    typeof(StructuredBreakpointProcessorViewModel)
                ]
            }
        ]);
    }

    /// <summary>
    /// Find the optimal archetype for a given breakpoint stream
    /// </summary>
    /// <returns>null if not appropriate</returns>
    public BreakpointDisplayArchetypeViewModel[] FindOptimalArchetypes(DebugBreakpointStreamMessage message)
    {
        return Archetypes
            .ToList()
            .OrderByDescending(x => x.Selector.GetPriority(message))
            .ToArray();
    }
}
