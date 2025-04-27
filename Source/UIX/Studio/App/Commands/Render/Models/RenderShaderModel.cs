using System.Collections.Generic;

namespace Studio.App.Commands.Render.Models;

public class RenderShaderModel
{
    /// <summary>
    /// Report model object
    /// </summary>
    public object Model { get; set; }
    
    /// <summary>
    /// Do we have symbols for this shader?
    /// </summary>
    public bool HasSymbols { get; set; }

    /// <summary>
    /// All files, requires symbols
    /// </summary>
    public List<RenderShaderFileModel> Files { get; set; } = new();
    
    /// <summary>
    /// All message objects
    /// </summary>
    public List<object> Messages { get; set; } = new();
    
    /// <summary>
    /// All rendered messages for inlining
    /// </summary>
    public List<string> MessageRenders { get; set; } = new();
}
