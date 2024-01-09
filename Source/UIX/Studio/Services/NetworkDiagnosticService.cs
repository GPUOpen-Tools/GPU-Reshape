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
using Avalonia;
using Avalonia.Threading;
using Bridge.CLR;
using ReactiveUI;
using Studio.Extensions;

namespace Studio.Services
{
    public class NetworkDiagnosticService : ReactiveObject
    {
        /// <summary>
        /// Total number of bytes read per second
        /// </summary>
        public double BytesReadPerSecond
        {
            get => _bytesReadPerSecond;
            set => this.RaiseAndSetIfChanged(ref _bytesReadPerSecond, value);
        }
        
        /// <summary>
        /// Total number of bytes written per second
        /// </summary>
        public double BytesWrittenPerSecond
        {
            get => _bytesWrittenPerSecond;
            set => this.RaiseAndSetIfChanged(ref _bytesWrittenPerSecond, value);
        }
        
        public NetworkDiagnosticService()
        {
            // Create timer on main thread
            _timer = new DispatcherTimer(DispatcherPriority.Background)
            {
                Interval = TimeSpan.FromMilliseconds(1000),
                IsEnabled = true
            };

            // Subscribe tick
            _timer.Tick += OnTick;
            
            // Must call start manually (a little vague)
            _timer.Start();
        }

        private void OnTick(object? sender, EventArgs e)
        {
            // Summarized
            BridgeInfo info = new BridgeInfo();
            
            // Accumulate info
            App.Locator.GetService<IWorkspaceService>()?.Workspaces.Items.ForEach(x =>
            {
                BridgeInfo workspaceInfo = x.Connection?.Bridge?.GetInfo() ?? new BridgeInfo();
                info.bytesWritten += workspaceInfo.bytesWritten;
                info.bytesRead += workspaceInfo.bytesRead;
            });

            // Average
            BytesReadPerSecond = Math.Max(0, (long)info.bytesRead - (long)_lastInfo.bytesRead) / _timer.Interval.TotalSeconds;
            BytesWrittenPerSecond = Math.Max(0, (long)info.bytesWritten - (long)_lastInfo.bytesWritten) / _timer.Interval.TotalSeconds;

            // Set last
            _lastInfo = info;
        }

        /// <summary>
        /// Last info state
        /// </summary>
        private BridgeInfo _lastInfo = new BridgeInfo();

        /// <summary>
        /// Internal read speed
        /// </summary>
        private double _bytesReadPerSecond;
        
        /// <summary>
        /// Internal write speed
        /// </summary>
        private double _bytesWrittenPerSecond;

        /// <summary>
        /// Internal timer
        /// </summary>
        private readonly DispatcherTimer _timer;
    }
}