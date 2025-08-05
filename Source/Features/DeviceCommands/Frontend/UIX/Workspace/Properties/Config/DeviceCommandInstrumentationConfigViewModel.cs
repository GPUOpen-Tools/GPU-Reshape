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
using Studio.Models.Workspace;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Properties.Config
{
    public class DeviceCommandInstrumentationConfigViewModel : BasePropertyViewModel, IBusObject
    {
        /// <summary>
        /// Feature info
        /// </summary>
        public FeatureInfo FeatureInfo { get; set; }
        
        /// <summary>
        /// Enables loop iteration limits
        /// </summary>
        [PropertyField]
        [Description("Limits the maximum number of dispatch group counts on any dimension")]
        public uint DispatchGroupLimit
        {
            get => _dispatchGroupLimit;
            set
            {
                this.RaiseAndSetIfChanged(ref _dispatchGroupLimit, value);
                this.EnqueueFirstParentBus();
            }
        }
        
        /// <summary>
        /// Default values
        /// </summary>
        public static readonly uint DefaultDispatchLimit = 32000000;

        /// <summary>
        /// Constructor
        /// </summary>
        public DeviceCommandInstrumentationConfigViewModel() : base("Instrumentation", PropertyVisibility.Configuration)
        {
            // Always commit initial state
            this.EnqueueFirstParentBus();
        }

        public void Commit(OrderedMessageView<ReadWriteMessageStream> stream)
        {
            var request = stream.Add<SetDeviceCommandInstrumentationConfigMessage>();
            request.dispatchGroupLimit = _dispatchGroupLimit;
        }

        /// <summary>
        /// Internal iteration limits
        /// </summary>
        private uint _dispatchGroupLimit = DefaultDispatchLimit;
    }
}