namespace Studio.App.Commands.Render.Models;

public class RenderMessageSummaryModel
{
    /// <summary>
    /// Total number of errors
    /// </summary>
    public int Errors { get; set; } = 0;

    /// <summary>
    /// Total number of warnings
    /// </summary>
    public int Warnings { get; set; } = 0;
    
    /// <summary>
    /// Total number of infos/notices
    /// </summary>
    public int Infos { get; set; } = 0;

    /// <summary>
    /// The summarized chart of the above
    /// </summary>
    public RenderMessageChartSummaryModel Chart { get; set; } = new();
}
