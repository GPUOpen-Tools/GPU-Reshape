using ReactiveUI;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Workspace.Objects;

namespace GRS.Features.Debug.UIX.ViewModels;

public class WatchpointSourceObject : ReactiveObject, ITextualSourceObject
{
    /// <summary>
    /// Source segment
    /// </summary>
    public required ShaderSourceSegment? Segment { get; set; }
    
    /// <summary>
    /// Display content of this watchpoint
    /// </summary>
    public required string Content { get; set; }

    /// <summary>
    /// Number of instances, just default it to zero for no display
    /// </summary>
    public uint Count { get; set; } = 0;

    /// <summary>
    /// Assigned severity, always "error" for red
    /// </summary>
    public SourceObjectSeverity Severity { get; set; } = SourceObjectSeverity.Error;

    /// <summary>
    /// Current detail view model
    /// </summary>
    public required ISourceObjectDetailViewModel? DetailViewModel
    {
        get => _detailViewModel;
        set => this.RaiseAndSetIfChanged(ref _detailViewModel, value);
    }

    /// <summary>
    /// Split off into their own category
    /// </summary>
    public string OverlayCategory { get; } = "Watchpoint";

    /// <summary>
    /// Internal detail state
    /// </summary>
    private ISourceObjectDetailViewModel? _detailViewModel;
}