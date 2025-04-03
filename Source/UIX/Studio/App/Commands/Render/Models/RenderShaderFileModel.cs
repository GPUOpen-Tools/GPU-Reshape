using System.Collections.Generic;

namespace Studio.App.Commands.Render.Models;

public class RenderShaderFileModel
{
    /// <summary>
    /// Report model object
    /// </summary>
    public object Model { get; set; }
    
    /// <summary>
    /// All messages associated with this file
    /// </summary>
    public List<object> Messages { get; set; } = new();
}
