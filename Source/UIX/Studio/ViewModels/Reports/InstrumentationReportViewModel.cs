using System.Collections.Generic;

namespace Studio.ViewModels.Reports
{
    public class InstrumentationReportViewModel
    {
        /// <summary>
        /// Number of passed shaders
        /// </summary>
        public int PassedShaders { get; set; }

        /// <summary>
        /// Number of failed shaders
        /// </summary>
        public int FailedShaders { get; set; }

        /// <summary>
        /// Number of passed pipelines
        /// </summary>
        public int PassedPipelines { get; set; }

        /// <summary>
        /// Number of failed pipelines
        /// </summary>
        public int FailedPipelines { get; set; }

        /// <summary>
        /// Shader compilation time (milliseconds)
        /// </summary>
        public int ShaderMilliseconds { get; set; }

        /// <summary>
        /// Shader compilation time (seconds)
        /// </summary>
        public float ShaderSeconds => ShaderMilliseconds / 1e3f;

        /// <summary>
        /// Pipeline compilation time (milliseconds)
        /// </summary>
        public int PipelineMilliseconds { get; set; }

        /// <summary>
        /// Pipeline compilation time (seconds)
        /// </summary>
        public float PipelineSeconds => PipelineMilliseconds / 1e3f;

        /// <summary>
        /// Total compilation time (milliseconds)
        /// </summary>
        public int TotalMilliseconds { get; set; }

        /// <summary>
        /// Total compilation time (seconds)
        /// </summary>
        public float TotalSeconds => TotalMilliseconds / 1e3f;

        /// <summary>
        /// All diagnostic messages
        /// </summary>
        public List<string> Messages { get; } = new();
    }
}