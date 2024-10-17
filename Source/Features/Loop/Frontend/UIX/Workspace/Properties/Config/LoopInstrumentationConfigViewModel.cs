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

using System.ComponentModel;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Models.Workspace;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Properties.Config
{
    public class LoopInstrumentationConfigViewModel : BasePropertyViewModel, IInstrumentationProperty
    {
        /// <summary>
        /// Feature info
        /// </summary>
        public FeatureInfo FeatureInfo { get; set; }
        
        /// <summary>
        /// Enables loop iteration limits
        /// </summary>
        [PropertyField]
        [Description("Limits the maximum number of loops iterations in a shader")]
        public bool UseIterationLimits
        {
            get => _useIterationLimits;
            set
            {
                this.RaiseAndSetIfChanged(ref _useIterationLimits, value);
                this.EnqueueFirstParentBus();
            }
        }
        
        /// <summary>
        /// If iteration limits are enabled, the maximum number of iterations
        /// </summary>
        [PropertyField]
        [Category("Loop")]
        [Description("The maximum number of loop iterations in a shader")]
        public uint IterationLimit
        {
            get => _iterationLimit;
            set
            {
                this.RaiseAndSetIfChanged(ref _iterationLimit, value);
                this.EnqueueFirstParentBus();
            }
        }
        
        /// <summary>
        /// The periodic interval for atomic checks
        /// </summary>
        [PropertyField]
        [Category("Loop")]
        [Description("The periodic iteration interval at which (atomic) termination signals are checked")]
        public uint AtomicIterationInterval
        {
            get => _atomicIterationInterval;
            set
            {
                this.RaiseAndSetIfChanged(ref _atomicIterationInterval, value);
                this.EnqueueFirstParentBus();
            }
        }
        
        /// <summary>
        /// Default values
        /// </summary>
        public static readonly bool DefaultUseIterationLimits = true;
        public static readonly uint DefaultIterationLimit = 32000;
        public static readonly uint DefaultAtomicIterationInterval = 256;

        /// <summary>
        /// Constructor
        /// </summary>
        public LoopInstrumentationConfigViewModel() : base("Instrumentation", PropertyVisibility.Configuration)
        {
            
        }

        /// <summary>
        /// Commit all changes
        /// </summary>
        /// <param name="state"></param>
        public void Commit(InstrumentationState state)
        {
            // Reduce stream size if not needed
            if (_useIterationLimits == DefaultUseIterationLimits &&
                _iterationLimit == DefaultIterationLimit &&
                _atomicIterationInterval == DefaultAtomicIterationInterval)
            {
                return;
            }
            
            // Submit request
            var request = state.GetOrDefault<SetLoopInstrumentationConfigMessage>();
            request.useIterationLimits |= _useIterationLimits ? 1 : 0;
            request.iterationLimit |= _iterationLimit;
            request.atomicIterationInterval |= _atomicIterationInterval;
        }

        /// <summary>
        /// Internal use limits state
        /// </summary>
        private bool _useIterationLimits = DefaultUseIterationLimits;

        /// <summary>
        /// Internal iteration limits
        /// </summary>
        private uint _iterationLimit = DefaultIterationLimit;

        /// <summary>
        /// Internal atomic iteration intervals
        /// </summary>
        private uint _atomicIterationInterval = DefaultAtomicIterationInterval;
    }
}