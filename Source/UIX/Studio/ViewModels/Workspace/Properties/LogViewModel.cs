﻿// 
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

using System;
using System.Collections.Generic;
using Avalonia;
using Avalonia.Threading;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Logging;

namespace Studio.ViewModels.Workspace.Properties
{
    public class LogViewModel : BasePropertyViewModel, Bridge.CLR.IBridgeListener
    {
        public LogViewModel() : base("Logs", PropertyVisibility.Default)
        {
            this.WhenAnyValue(x => x.ConnectionViewModel).Subscribe(_ => OnConnectionChanged());
        }
        
        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            // Register to logging messages
            ConnectionViewModel?.Bridge?.Register(LogMessage.ID, this);

            // Create all properties
            CreateProperties();
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public override void Destruct()
        {
            // Remove listeners
            ConnectionViewModel?.Bridge?.Deregister(LogMessage.ID, this);
        }

        /// <summary>
        /// Create all properties
        /// </summary>
        private void CreateProperties()
        {
            Properties.Clear();

            if (ConnectionViewModel == null)
            {
                return;
            }
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsDynamic(LogMessage.ID))
            {
                return;
            }

            List<Tuple<LogSeverity, string>> messages = new();

            // Enumerate all messages
            foreach (LogMessage message in new DynamicMessageView<LogMessage>(streams))
            {
                messages.Add(Tuple.Create((LogSeverity)message.severity, $"{ConnectionViewModel?.Application?.Name} - [{message.system.String}] {message.message.String}"));
            }

            // Append messages on UI thread
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                foreach (var message in messages)
                {
                    _loggingViewModel?.Events.Add(new LogEvent()
                    {
                        Severity = message.Item1,
                        Message = message.Item2
                    });
                }
            });
        }

        /// <summary>
        /// Shared logging view model
        /// </summary>
        private ILoggingViewModel? _loggingViewModel = App.Locator.GetService<ILoggingService>()?.ViewModel;
    }
}