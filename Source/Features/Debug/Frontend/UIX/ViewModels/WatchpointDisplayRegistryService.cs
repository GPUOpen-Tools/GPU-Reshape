using System;
using System.Linq;
using System.Collections.ObjectModel;
using AvaloniaEdit.Utils;
using GRS.Features.Debug.UIX.ViewModels.Processor;
using GRS.Features.Debug.UIX.ViewModels.Selectors;
using Message.CLR;

namespace GRS.Features.Debug.UIX.ViewModels;

public class WatchpointDisplayRegistryService
{
    /// <summary>
    /// All archetypes
    /// </summary>
    public ObservableCollection<WatchpointDisplayArchetypeViewModel> Archetypes { get; set; } = new();

    public WatchpointDisplayRegistryService()
    {
        // Setup the standard archetypes
        Archetypes.AddRange([
            // Standard image view
            new WatchpointDisplayArchetypeViewModel
            {
                Name = "Image",
                Archetype = typeof(ImageWatchpointDisplayViewModel),
                Selector = new ImageWatchpointDisplaySelectorViewModel(),
                ProcessorArchetypes = 
                [
                    typeof(ImageWatchpointProcessorViewModel)
                ]
            },
            
            // Loose display
            new WatchpointDisplayArchetypeViewModel
            {
                Name = "Loose",
                Archetype = typeof(LooseWatchpointDisplayViewModel),
                Selector = new LooseWatchpointDisplaySelectorViewModel(),
                ProcessorArchetypes = 
                [
                    typeof(LooseWatchpointProcessorViewModel)
                ]
            }
        ]);
    }

    /// <summary>
    /// Find the optimal archetype for a given watchpoint stream
    /// </summary>
    /// <returns>null if not appropriate</returns>
    public WatchpointDisplayArchetypeViewModel[] FindOptimalArchetypes(DebugWatchpointStreamMessage message)
    {
        return Archetypes
            .ToList()
            .Select(x => Tuple.Create(x.Selector.GetPriority(message), x))
            .Where(x => x.Item1 != WatchpointDisplaySelectorPriority.Unsupported)
            .OrderByDescending(x => x.Item1)
            .Select(x  => x.Item2)
            .ToArray();
    }
}
