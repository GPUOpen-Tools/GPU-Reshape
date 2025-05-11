using System.Collections.Generic;
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
    public BreakpointDisplayArchetypeViewModel? FindOptimalArchetype(DebugBreakpointStreamMessage message)
    {
        KeyValuePair<int, BreakpointDisplayArchetypeViewModel?> candidate = new(0, null);

        // Go through each archetype, try to see if it fits or not
        // Always overwrite on the same priorities, makes external extensions easier
        foreach (BreakpointDisplayArchetypeViewModel archetypeViewModel in Archetypes)
        {
            if (archetypeViewModel.Selector.GetPriority(message) is { } priority && priority >= candidate.Key)
            {
                candidate = new(priority, archetypeViewModel);
            }
        }

        return candidate.Value;
    }
}
