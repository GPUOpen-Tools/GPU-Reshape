// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Models.Workspace;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Properties.Config
{
    public class InstrumentationConfigViewModel : BasePropertyViewModel, IInstrumentationProperty
    {
        /// <summary>
        /// Feature info
        /// </summary>
        public FeatureInfo FeatureInfo { get; set; }
        
        /// <summary>
        /// Enables safe-guarding on potentially offending instructions
        /// </summary>
        [PropertyField]
        public bool SafeGuard
        {
            get => _safeGuard;
            set
            {
                this.RaiseAndSetIfChanged(ref _safeGuard, value);
                this.EnqueueFirstParentBus();
            }
        }
        
        /// <summary>
        /// Enables detailed instrumentation
        /// </summary>
        [PropertyField]
        public bool Detail
        {
            get => _detail;
            set
            {
                this.RaiseAndSetIfChanged(ref _detail, value);
                this.EnqueueFirstParentBus();
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public InstrumentationConfigViewModel() : base("Instrumentation", PropertyVisibility.Configuration)
        {
            
        }

        /// <summary>
        /// Commit all changes
        /// </summary>
        /// <param name="state"></param>
        public void Commit(InstrumentationState state)
        {
            // Reduce stream size if not needed
            if (!_safeGuard && !_detail)
            {
                return;
            }
            
            // Submit request
            var request = state.GetOrDefault<SetInstrumentationConfigMessage>();
            request.safeGuard |= _safeGuard ? 1 : 0;
            request.detail |= _detail ? 1 : 0;
        }

        /// <summary>
        /// Internal safe-guard state
        /// </summary>
        private bool _safeGuard = false;

        /// <summary>
        /// Internal detail state
        /// </summary>
        private bool _detail = false;
    }
}