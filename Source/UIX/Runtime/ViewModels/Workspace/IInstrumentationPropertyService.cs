﻿// 
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

using System.Threading.Tasks;
using Studio.Models.Instrumentation;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace
{
    public interface IInstrumentationPropertyService : IPropertyService
    {
        /// <summary>
        /// Name of this feature
        /// </summary>
        public string Name { get; }
        
        /// <summary>
        /// Descriptive category assigned to this feature
        /// </summary>
        public string Category { get; }

        /// <summary>
        /// All flags of this feature
        /// </summary>
        public InstrumentationFlag Flags { get; }

        /// <summary>
        /// Check if instrumentation may be applied to a given target
        /// </summary>
        /// <param name="instrumentable"></param>
        /// <returns>false if not valid</returns>
        public bool IsInstrumentationValidFor(IInstrumentableObject instrumentable);

        /// <summary>
        /// Create an instrumentation property for a given target
        /// </summary>
        /// <param name="target"></param>
        /// <param name="replication"></param>
        /// <returns>async task</returns>
        public Task<IPropertyViewModel?> CreateInstrumentationObjectProperty(IPropertyViewModel target, bool replication);
    }
}
