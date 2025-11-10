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

using System;

namespace Message.CLR
{
    /// <summary>
    /// Mirrors backend flags
    /// </summary>
    [Flags]
    public enum ExecutionFlag
    {
        Draw       = 1 << 0,
        Dispatch   = 1 << 1,
        Raytracing = 1 << 2,
        Indirect   = 1 << 3
    }
    
    [Flags]
    public enum ExecutionDrawFlag {
        VertexCountPerInstance = 1 << 0,
        IndexCountPerInstance = 1 << 1,
        InstanceCount = 1 << 2,
        StartVertex = 1 << 3,
        StartIndex = 1 << 4,
        StartInstance = 1 << 5,
        VertexOffset = 1 << 6,
        InstanceOffset = 1 << 7
    };
    
    public struct Traceback
    {
        /// <summary>
        /// Combined execution flags of the program
        /// </summary>
        public ExecutionFlag executionFlag { get; set; }
    
        /// <summary>
        /// Launching pipeline
        /// </summary>
        public uint pipelineUid { get; set; }
    
        /// <summary>
        /// Combined scope
        /// </summary>
        public uint[] markerHashes32 { get; set; }
    
        /// <summary>
        /// Scheduled queue
        /// </summary>
        public uint queueUid { get; set; }
    
        /// <summary>
        /// Launch parameters X
        /// </summary>
        public uint kernelLaunchX { get; set; }
    
        /// <summary>
        /// Launch parameters Y
        /// </summary>
        public uint kernelLaunchY { get; set; }
    
        /// <summary>
        /// Launch parameters Z
        /// </summary>
        public uint kernelLaunchZ { get; set; }
    
        /// <summary>
        /// Local thread X
        /// </summary>
        public uint threadX { get; set; }
    
        /// <summary>
        /// Local thread Y
        /// </summary>
        public uint threadY { get; set; }
    
        /// <summary>
        /// Local thread Z
        /// </summary>
        public uint threadZ { get; set; }
    }
}
