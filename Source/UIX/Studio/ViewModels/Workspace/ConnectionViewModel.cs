// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
using System.Collections.Generic;
using System.Diagnostics;
using System.Reactive;
using System.Reactive.Subjects;
using System.Threading;
using Avalonia;
using Avalonia.Controls.Notifications;
using Avalonia.Platform;
using Avalonia.Threading;
using Bridge.CLR;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Workspace;

namespace Studio.ViewModels.Workspace
{
    public class ConnectionViewModel : ReactiveObject, IConnectionViewModel
    {
        /// <summary>
        /// Invoked during connection
        /// </summary>
        public ISubject<Unit> Connected { get; }

        /// <summary>
        /// Invoked during connection rejection
        /// </summary>
        public ISubject<Unit> Refused { get; }

        /// <summary>
        /// Local time on the remote endpoint
        /// </summary>
        public DateTime LocalTime
        {
            get => localTime;
            set => this.RaiseAndSetIfChanged(ref localTime, value);
        }

        /// <summary>
        /// Underlying bridge of this connection
        /// </summary>
        public IBridge? Bridge
        {
            get
            {
                lock (this)
                {
                    return _remote;
                }
            }
        }

        /// <summary>
        /// Is the connection alive?
        /// </summary>
        public bool IsConnected => _remote != null;

        /// <summary>
        /// Endpoint application info
        /// </summary>
        public ApplicationInfoViewModel? Application { get; private set; }

        public ConnectionViewModel()
        {
            // Create subjects
            Connected = new Subject<Unit>();
            Refused = new Subject<Unit>();

            // Create dispatcher local to the connection
            _dispatcher = new Dispatcher(AvaloniaLocator.Current.GetService<IPlatformThreadingInterface>());

            // Create timer on main thread
            _timer = new DispatcherTimer(DispatcherPriority.Background)
            {
                Interval = TimeSpan.FromMilliseconds(15),
                IsEnabled = true
            };

            // Subscribe tick
            _timer.Tick += OnTick;

            // Must call start manually (a little vague)
            _timer.Start();
        }

        /// <summary>
        /// Invoked during timer ticks
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnTick(object? sender, EventArgs e)
        {
            // Commit all changes
            Commit();
        }

        /// <summary>
        /// Connect synchronously
        /// </summary>
        /// <param name="ipvx">remote endpoint</param>
        /// <param name="port">optional</param>
        public void Connect(string ipvx, int? port)
        {
            lock (this)
            {
                // Close existing connection and cancel pending requests
                _remote?.Stop();

                // Create bridge
                if (_remote == null)
                {
                    // Create new RC bridge
                    _remote = new RemoteClientBridge();
                
                    // Always commit when remote append streams
                    _remote.SetCommitOnAppend(true);
                
                    // Set async handler
                    _remote.SetAsyncConnectedDelegate(() =>
                    {
                        Connected.OnNext(Unit.Default);
                    });
                }

                // Create config
                var config = new EndpointConfig();

                // Optional port
                if (port.HasValue)
                {
                    config.sharedPort = (UInt32)port;
                }

                // Install against endpoint
                _remote.InstallAsync(new EndpointResolve()
                {
                    config = config,
                    ipvxAddress = ipvx
                });
            }
        }

        /// <summary>
        /// Submit discovery request asynchronously
        /// </summary>
        public void DiscoverAsync()
        {
            lock (this)
            {
                _remote?.DiscoverAsync();
            }
        }

        /// <summary>
        /// Submit client request asynchronously
        /// </summary>
        /// <param name="guid">endpoint application GUID</param>
        public void RequestClientAsync(ApplicationInfoViewModel applicationInfo)
        {
            lock (this)
            {
                // Set info
                Application = applicationInfo;

                // Pass down CLR
                _remote?.RequestClientAsync(applicationInfo.Guid);
            }
        }

        /// <summary>
        /// Get the shared bus within this connection
        /// </summary>
        /// <returns></returns>
        public OrderedMessageView<ReadWriteMessageStream> GetSharedBus()
        {
            // Allocate ordered if not ready
            if (_sharedBus == null)
            {
                _sharedBus = new OrderedMessageView<ReadWriteMessageStream>(new ReadWriteMessageStream());
            }

            // OK
            return _sharedBus;
        }

        /// <summary>
        /// Commit all pending bus messages
        /// </summary>
        public void Commit()
        {
            // Submit current bus
            if (_sharedBus != null)
            {
                Bridge?.GetOutput().AddStream(_sharedBus.Storage);
            }

            // Commit pending changes
            Bridge?.Commit();

            // Release
            _sharedBus = null;
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            lock (this)
            {
                _remote?.Cancel();
                _remote?.Stop();
                _remote = null;
            }
        }

        /// <summary>
        /// Internal dispatcher
        /// </summary>
        private Dispatcher _dispatcher;

        /// <summary>
        /// Shared bus timer
        /// </summary>
        private DispatcherTimer _timer;

        /// <summary>
        /// Shared bus for convenient messaging
        /// </summary>
        private OrderedMessageView<ReadWriteMessageStream>? _sharedBus;

        /// <summary>
        /// Internal underlying bridge
        /// </summary>
        private Bridge.CLR.RemoteClientBridge? _remote;

        /// <summary>
        /// Internal local time
        /// </summary>
        private DateTime localTime;
    }
}