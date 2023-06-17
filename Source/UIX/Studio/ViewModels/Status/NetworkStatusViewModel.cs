// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Status
{
    public class NetworkStatusViewModel : ReactiveObject, IStatusViewModel
    {
        /// <summary>
        /// Standard orientation
        /// </summary>
        public StatusOrientation Orientation => StatusOrientation.Right;

        /// <summary>
        /// Read string
        /// </summary>
        public string ReadAmount
        {
            get => _readAmount;
            set => this.RaiseAndSetIfChanged(ref _readAmount, value);
        }

        /// <summary>
        /// Write string
        /// </summary>
        public string WrittenAmount
        {
            get => _writtenAmount;
            set => this.RaiseAndSetIfChanged(ref _writtenAmount, value);
        }

        /// <summary>
        /// Read icon opacity
        /// </summary>
        public double ReadOpacity
        {
            get => _readOpacity;
            set => this.RaiseAndSetIfChanged(ref _readOpacity, value);
        }

        /// <summary>
        /// Write icon opacity
        /// </summary>
        public double WriteOpacity
        {
            get => _writeOpacity;
            set => this.RaiseAndSetIfChanged(ref _writeOpacity, value);
        }

        /// <summary>
        /// Decorate a measure
        /// </summary>
        private string DecorateByteCount(double bytes)
        {
            // Small nudge beyond the prefix window
            const float edgeFactor = 1.25f;

            return bytes switch
            {
                < 1e3 * edgeFactor => $"{(uint)(bytes)} B/s",
                < 1e6 * edgeFactor => $"{(uint)(bytes / 1e3)} KB/s",
                < 1e9 * edgeFactor => $"{(uint)(bytes / 1e6)} MB/s",
                _ => $"{(uint)(bytes / 1e9)} GB/s"
            };
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public NetworkStatusViewModel()
        {
            // Bind diagnostics
            _networkDiagnosticService?.WhenAnyValue(x => x.BytesReadPerSecond, x => x.BytesWrittenPerSecond).Subscribe(x =>
            {
                // Set strings
                ReadAmount = DecorateByteCount(x.Item1);
                WrittenAmount = DecorateByteCount(x.Item2);
                
                // Update opacity
                ReadOpacity = Math.Min(1.0f, 0.25f + x.Item1 / 1e6);
                WriteOpacity = Math.Min(1.0f, 0.25f + x.Item2 / 1e6);
            });
        }
        
        /// <summary>
        /// Network service
        /// </summary>
        private NetworkDiagnosticService? _networkDiagnosticService = App.Locator.GetService<NetworkDiagnosticService>();

        /// <summary>
        /// Internal read amount
        /// </summary>
        private string _readAmount = "";
        
        /// <summary>
        /// Internal write amount
        /// </summary>
        private string _writtenAmount = "";

        /// <summary>
        /// Internal read opacity
        /// </summary>
        private double _readOpacity = 0.25f;
        
        /// <summary>
        /// Internal write opacity
        /// </summary>
        private double _writeOpacity = 0.25f;
    }
}