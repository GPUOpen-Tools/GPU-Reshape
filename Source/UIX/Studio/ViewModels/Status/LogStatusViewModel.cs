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
using System.Reactive.Linq;
using Avalonia;
using Bridge.CLR;
using DynamicData;
using DynamicData.Binding;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Logging;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Status
{
    public class LogStatusViewModel : ReactiveObject, IStatusViewModel
    {
        /// <summary>
        /// Standard orientation
        /// </summary>
        public StatusOrientation Orientation => StatusOrientation.Left;

        /// <summary>
        /// Current status message to be displayed
        /// </summary>
        public string Status
        {
            get => _status;
            set => this.RaiseAndSetIfChanged(ref _status, value);
        }

        public LogStatusViewModel()
        {
            ILoggingViewModel? logging = ServiceRegistry.Get<ILoggingService>()?.ViewModel;
            
            // Connect to workspaces
            logging?.Events.Connect()
                .OnItemAdded(OnLog)
                .Subscribe();
        }

        /// <summary>
        /// Invoked on logging
        /// </summary>
        /// <param name="logEvent"></param>
        private void OnLog(LogEvent logEvent)
        {
            Status = logEvent.Message;
        }

        /// <summary>
        /// Internal status
        /// </summary>
        private string _status = string.Empty;
    }
}