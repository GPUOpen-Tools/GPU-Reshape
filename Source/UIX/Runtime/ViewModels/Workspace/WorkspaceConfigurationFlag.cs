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

namespace Studio.ViewModels.Workspace
{
    [Flags]
    public enum WorkspaceConfigurationFlag
    {
        /// <summary>
        /// No flags
        /// </summary>
        None = 0,
        
        /// <summary>
        /// Supports detailed reporting
        /// </summary>
        CanDetail = (1 << 0),

        /// <summary>
        /// May be safe guarded
        /// </summary>
        CanSafeGuard = (1 << 1),
        
        /// <summary>
        /// Requires synchronous reporting for valid instrumentation
        /// </summary>
        RequiresSynchronousRecording = (1 << 2),
        
        /// <summary>
        /// Supports texel addressing
        /// </summary>
        CanUseTexelAddressing = (1 << 3),
        
        /// <summary>
        /// All valid flags
        /// </summary>
        All = (1 << 4) - 1,
        
        /// <summary>
        /// Binding only, negates the test
        /// </summary>
        BindingNegation = (1 << 4)
    }

    public static class WorkspaceConfigurationFlagBindings
    {
        /// <summary>
        /// Recording binding
        /// </summary>
        public static WorkspaceConfigurationFlag RequiresSynchronousRecordingBinding =
            WorkspaceConfigurationFlag.RequiresSynchronousRecording | 
            WorkspaceConfigurationFlag.BindingNegation;
    }
}