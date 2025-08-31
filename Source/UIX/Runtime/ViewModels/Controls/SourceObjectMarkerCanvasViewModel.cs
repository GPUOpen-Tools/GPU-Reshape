using System.Collections.ObjectModel;
using System.Windows.Input;
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
}
