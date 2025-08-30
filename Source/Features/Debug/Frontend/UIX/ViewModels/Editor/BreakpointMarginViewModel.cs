using ReactiveUI;
using Studio.ViewModels.Shader;

namespace GRS.Features.Debug.UIX.ViewModels.Editor;

public class BreakpointMarginViewModel : ReactiveObject
{
    /// <summary>
    /// The textual view model
    /// </summary>
    public required ITextualShaderContentViewModel ContentViewModel { get; set; }
    
    /// <summary>
    /// The target collection
    /// </summary>
    public required ShaderBreakpointCollectionViewModel CollectionViewModel { get; set; }
    
    /// <summary>
    /// The current breakpoint view model
    /// </summary>
    public BreakpointViewModel? HighlightedBreakpointViewModel { get; set; }
    
    /// <summary>
    /// The current line number
    /// </summary>
    public int LineNumberBase0 { get; set; }
}
