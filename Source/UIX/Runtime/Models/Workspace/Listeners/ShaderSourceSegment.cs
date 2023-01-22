using Studio.Models.Workspace.Objects;

namespace Studio.Models.Workspace.Listeners
{
    public class ShaderSourceSegment
    {
        /// <summary>
        /// Contained shader extract
        /// </summary>
        public string? Extract { get; set; }
        
        /// <summary>
        /// Location of this segment
        /// </summary>
        public ShaderLocation Location { get; set; }
    }
}