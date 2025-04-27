// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

using System.Collections.Generic;
using Runtime.ViewModels.Traits;

namespace Studio.ViewModels.Reports
{
    public class InstrumentationReportViewModel : ISerializable
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

        /// <summary>
        /// Serialize this object
        /// </summary>
        public object Serialize()
        {
            return new SerializationMap()
            {
                { "PassedShaders", PassedShaders },
                { "FailedShaders", FailedShaders },
                { "PassedPipelines", PassedPipelines },
                { "FailedPipelines", FailedPipelines },
                { "ShaderMilliseconds", ShaderMilliseconds },
                { "ShaderSeconds", ShaderSeconds },
                { "PipelineMilliseconds", PipelineMilliseconds },
                { "TotalMilliseconds", TotalMilliseconds },
                { "TotalSeconds", TotalSeconds },
                { "Messages", Messages }
            };
        }
    }
}