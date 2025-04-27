using System.Collections.Generic;
using System.Drawing;

namespace Studio.App.Commands.Render.Models;

public class RenderMessageChartDataModel
{
    /// <summary>
    /// Validation contents of this data item
    /// </summary>
    public string Content { get; set; } = string.Empty;
    
    /// <summary>
    /// Number of occurrances of this data item
    /// </summary>
    public int Count { get; set; } = 0;
    
    /// <summary>
    /// Assigned rendering color
    /// </summary>
    public Color Color { get; set; }
}

public class RenderMessageChartSummaryModel
{
    /// <summary>
    /// All data items of this chart
    /// </summary>
    public List<RenderMessageChartDataModel> Data = new();
}
