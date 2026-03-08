using System;
using System.Collections.ObjectModel;
using System.Windows.Input;
using DynamicData;
using DynamicData.Binding;
using Studio.Services;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Controls;

public class SourceObjectMarkerCanvasViewModel
{
    /// <summary>
    /// All objects in this canvas
    /// </summary>
    public ObservableCollection<ITextualSourceObject> SourceObjects { get; } = new();

    /// <summary>
    /// Detail command to propagate to the markers
    /// </summary>
    public ICommand? DetailCommand { get; set; }

    public SourceObjectMarkerCanvasViewModel()
    {
        SourceObjects
            .ToObservableChangeSet()
            .OnItemRemoved(OnObjectRemoved)
            .Subscribe();
    }

    /// <summary>
    /// Invoked on removals
    /// </summary>
    private void OnObjectRemoved(ITextualSourceObject obj)
    {
        // Reset the selection state if matching
        if (ServiceRegistry.Get<ISourceService>() is { } sourceService)
        {
            if (sourceService.SelectedSourceObject == obj)
            {
                sourceService.SelectedSourceObject = null;
            }
        }
    }
}
