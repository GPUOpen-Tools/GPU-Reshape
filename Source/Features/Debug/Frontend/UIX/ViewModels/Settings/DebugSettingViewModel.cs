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

using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.ViewModels.Setting;
using Studio.Services.Suspension;

namespace GRS.Features.Debug.UIX.Settings
{
    public class DebugSettingViewModel : BaseSettingViewModel, INotifySuspension
    {
        /// <summary>
        /// Max number of Mb a breakpoint may use
        /// </summary>
        [DataMember]
        public uint MaxBreakpointMemoryMb
        {
            get => _maxBreakpointMemoryMb;
            set => this.RaiseAndSetIfChanged(ref _maxBreakpointMemoryMb, value);
        }
        
        /// <summary>
        /// Initial number of Mb a breakpoint may use
        /// </summary>
        [DataMember]
        public uint DefaultBreakpointMemoryMb
        {
            get => _defaultBreakpointMemoryMb;
            set => this.RaiseAndSetIfChanged(ref _defaultBreakpointMemoryMb, value);
        }
        
        /// <summary>
        /// Max dimension of an image display
        /// </summary>
        [DataMember]
        public uint MaxBreakpointImageSizePerAxis
        {
            get => _maxBreakpointImageSizePerAxisSize;
            set => this.RaiseAndSetIfChanged(ref _maxBreakpointImageSizePerAxisSize, value);
        }

        public DebugSettingViewModel() : base("Debugging")
        {
            
        }

        public void Suspending()
        {
            
        }

        /// <summary>
        /// Internal max mb
        /// </summary>
        private uint _maxBreakpointMemoryMb = 64;

        /// <summary>
        /// Internal max mb
        /// </summary>
        private uint _maxBreakpointImageSizePerAxisSize = 4096;

        /// <summary>
        /// Internal initial mb
        /// </summary>
        private uint _defaultBreakpointMemoryMb = 8;
    }
}