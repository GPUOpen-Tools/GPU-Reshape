namespace Studio.App.Commands.Render.Models;

public class RenderInstrumentationSummaryModel
{
    /// <summary>
    /// Number of passed shaders
    /// </summary>
    public int PassedShaders { get; set; } = 0;
    
    /// <summary>
    /// Number of passed pipelines
    /// </summary>
    public int PassedPipelines { get; set; } = 0;
    
    /// <summary>
    /// Number of failed shaders
    /// </summary>
    public int FailedShaders { get; set; } = 0;
    
    /// <summary>
    /// Number of failed pipelines
    /// </summary>
    public int FailedPipelines { get; set; } = 0;
    
    /// <summary>
    /// Total seconds spent compiling shaders
    /// </summary>
    public float ShaderSeconds { get; set; } = 0;
    
    /// <summary>
    /// Total number of seconds spent compiling pipelines
    /// </summary>
    public float PipelineSeconds { get; set; } = 0;
}
