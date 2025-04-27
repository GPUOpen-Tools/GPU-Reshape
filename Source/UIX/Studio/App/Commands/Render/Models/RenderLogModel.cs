namespace Studio.App.Commands.Render.Models;

public class RenderLogModel
{
    /// <summary>
    /// Given severity
    /// </summary>
    public string Severity { get; set; } = string.Empty;

    /// <summary>
    /// Reporting system
    /// </summary>
    public string System { get; set; } = string.Empty;

    /// <summary>
    /// Log contents
    /// </summary>
    public string Message { get; set; } = string.Empty;
    
    /// <summary>
    /// Optional, view model for the log
    /// </summary>
    public object? ViewModel { get; set; } = null;
}
