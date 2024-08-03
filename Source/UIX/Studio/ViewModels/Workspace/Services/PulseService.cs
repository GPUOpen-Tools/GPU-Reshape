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
using System.Net.NetworkInformation;
using Avalonia;
using Avalonia.Threading;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Logging;
using Studio.Services;

namespace Studio.ViewModels.Workspace.Services
{
    public class PulseService : ReactiveObject, IPulseService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Last known pulse time
        /// </summary>
        public DateTime LastPulseTime
        {
            get => _lastPulseTime; 
            set => this.RaiseAndSetIfChanged(ref _lastPulseTime, value);
        }
        
        /// <summary>
        /// Has the pulse been missed for an unreasonable amount of time?
        /// </summary>
        public bool MissedPulse
        {
            get => _missedPulse; 
            set => this.RaiseAndSetIfChanged(ref _missedPulse, value);
        }

        /// <summary>
        /// Target connection
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _connectionViewModel, value);

                OnConnectionChanged();
            }
        }

        public PulseService()
        {
            // Create timer on main thread
            _timer = new DispatcherTimer(DispatcherPriority.Background)
            {
                Interval = TimeSpan.FromSeconds(1),
                IsEnabled = true
            };

            // Subscribe tick
            _timer.Tick += OnTick;
            
            // Must call start manually
            _timer.Start();
        }

        private void OnTick(object? sender, EventArgs e)
        {
            Refresh();
        }

        public void Refresh()
        {
            // Valid connection?
            if (_connectionViewModel?.Bridge == null)
            {
                return;
            }
            
            // Check last known time, report lost if needed
            UpdateMissedPulse();
            
            // Create stream
            var stream = new StaticMessageView<PingPongMessage, ReadWriteMessageStream>(new ReadWriteMessageStream());

            // Add local ping
            var message = stream.Add();
            message.timeStamp = EncodeDateTime(DateTime.Now);
            
            // Submit and wait for pong
            _connectionViewModel.Bridge?.GetOutput().AddStream(stream.Storage);
        }

        public void UpdateMissedPulse()
        {
            // Misssed if more than 10 seconds has passed
            bool missedPulse = (DateTime.Now - _lastPulseTime).TotalSeconds > 10;

            // Changed?
            if (missedPulse != MissedPulse)
            {
                var logger = ServiceRegistry.Get<ILoggingService>();
                
                // Log event
                if (missedPulse)
                {
                    logger?.ViewModel.Events.Add(new LogEvent()
                    {
                        Severity = LogSeverity.Warning,
                        Message = $"Lost connection to {ConnectionViewModel?.Application?.DecoratedName ?? "Unknown"}"
                    });
                }
                else
                {
                    logger?.ViewModel.Events.Add(new LogEvent()
                    {
                        Severity = LogSeverity.Info,
                        Message = $"Regained connection to {ConnectionViewModel?.Application?.DecoratedName ?? "Unknown"}"
                    });
                }

                // OK
                MissedPulse = missedPulse;
            }
        }
        
        /// <summary>
        /// Encode the date time
        /// </summary>
        /// <param name="time">base date time</param>
        /// <returns></returns>
        private unsafe ulong EncodeDateTime(DateTime time)
        {
            long value = time.ToBinary();
            return *(ulong*)&value;
        }

        /// <summary>
        /// Decode a date time
        /// </summary>
        /// <param name="data">encoded date time</param>
        /// <returns></returns>
        private unsafe DateTime DecodeDateTime(ulong data)
        {
            return DateTime.FromBinary(*(long*)&data);
        }

        /// <summary>
        /// Invoked on connection changes
        /// </summary>
        private void OnConnectionChanged()
        {
            _connectionViewModel?.Bridge?.Register(PingPongMessage.ID, this);
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Remove listeners
            _connectionViewModel?.Bridge?.Deregister(PingPongMessage.ID, this);
        }
        
        /// <summary>
        /// Invoked on message streams
        /// </summary>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            foreach (PingPongMessage message in new StaticMessageView<PingPongMessage>(streams))
            {
                DateTime time = DecodeDateTime(message.timeStamp);

                Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (time > LastPulseTime)
                    {
                        LastPulseTime = time;
                        UpdateMissedPulse();
                    } 
                });
            }
        }

        /// <summary>
        /// Internal connection state
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Internal pulse time
        /// </summary>
        private DateTime _lastPulseTime = DateTime.Now;
        
        /// <summary>
        /// Pooling timer
        /// </summary>
        private DispatcherTimer _timer;

        /// <summary>
        /// Internal miss state
        /// </summary>
        private bool _missedPulse = false;
    }
}