using ReactiveUI;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Objects;

public interface ITextualSourceObject : IReactiveObject
{
    /// <summary>
    /// Segment of this object
    /// </summary>
    public ShaderSourceSegment? Segment { get; set; }
    
    /// <summary>
    /// Assigned content
    /// </summary>
    public string Content { get; set; }
    
    /// <summary>
    /// Number of instances of this object
    /// </summary>
    public uint Count { get; set; }
    
    /// <summary>
    /// Severity of this validation object
    /// </summary>
    public SourceObjectSeverity Severity { get; set; }
    
    /// <summary>
    /// Optional, the detail view model of this object
    /// </summary>
    public ISourceObjectDetailViewModel? DetailViewModel { get; set; }
    
    /// <summary>
    /// The category of this object, objects with the same category get grouped
    /// </summary>
    public string OverlayCategory { get; }
}
