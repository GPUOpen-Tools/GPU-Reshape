using ReactiveUI;
using Studio.ViewModels.Shader;

namespace GRS.Features.Debug.UIX.ViewModels.Editor;

public class WatchpointMarginViewModel : ReactiveObject
{
    /// <summary>
    /// The textual view model
    /// </summary>
    public required ITextualContent Content { get; set; }
    
    /// <summary>
    /// The target collection
    /// </summary>
    public required WatchpointCollectionViewModel CollectionViewModel { get; set; }
    
    /// <summary>
    /// The current watchpoint view model
    /// </summary>
    public WatchpointViewModel? HighlightedWatchpointViewModel { get; set; }
    
    /// <summary>
    /// The current line number
    /// </summary>
    public int LineNumberBase0 { get; set; }
    
    /// <summary>
    /// The current or last focused line number
    /// </summary>
    public int LastFocusLineNumberBase0 { get; set; }
}
