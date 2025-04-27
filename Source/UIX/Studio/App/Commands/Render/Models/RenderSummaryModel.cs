namespace Studio.App.Commands.Render.Models;

public class RenderSummaryModel
{
    /// <summary>
    /// The launched process name
    /// </summary>
    public string PrimaryProcessName { get; set; } = "Unknown";

    /// <summary>
    /// The assigned configuration
    /// </summary>
    public string PrimaryProcessConfiguration { get; set; } = "None";
    
    /// <summary>
    /// Did we instrument with detail?
    /// </summary>
    public bool Detail { get; set; } = false;
    
    /// <summary>
    /// Did we instrument with coverage?
    /// </summary>
    public bool Coverage { get; set; } = false;
    
    /// <summary>
    /// Did we instrument with synchronous recording?
    /// </summary>
    public bool SynchronousRecording { get; set; } = false;
    
    /// <summary>
    /// Did we instrument with texel addressing?
    /// </summary>
    public bool TexelAddressing { get; set; } = false;
    
    /// <summary>
    /// Did we instrument with safe guarding?
    /// </summary>
    public bool SafeGuard { get; set; } = false;
    
    /// <summary>
    /// Instrumentation summary
    /// </summary>
    public RenderInstrumentationSummaryModel Instrumentation { get; set; } = new();
    
    /// <summary>
    /// Message summary
    /// </summary>
    public RenderMessageSummaryModel Message { get; set; } = new();
}
