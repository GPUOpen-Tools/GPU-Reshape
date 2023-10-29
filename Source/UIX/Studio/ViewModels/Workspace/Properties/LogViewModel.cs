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

using System;
using System.Collections.Generic;
using System.Diagnostics;
using Avalonia;
using Avalonia.Threading;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Logging;
using Studio.ViewModels.Reports;

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
            // Register to messages
            ConnectionViewModel?.Bridge?.Register(this);
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
            List<LogEvent> events = new();

            // If ordered, handle per message
            if (streams.GetSchema().IsOrdered())
            {
                foreach (OrderedMessage message in new OrderedMessageView(streams))
                {
                    switch (message.ID)
                    {
                        case InstrumentationDiagnosticMessage.ID:
                            Handle(message.Get<InstrumentationDiagnosticMessage>(), events);
                            break;
                    }
                }
            }
            else
            {
                // Dynamic / static messages
                switch (streams.GetSchema().id)
                {
                    case LogMessage.ID:
                        Handle(new DynamicMessageView<LogMessage>(streams), events);
                        break;
                }  
            }

            // Nothing to append?
            if (events.Count == 0)
            {
                return;
            }

            // Append messages on UI thread
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                foreach (LogEvent instance in events)
                {
                    _loggingViewModel?.Events.Add(instance);
                }
            });
        }

        /// <summary>
        /// Handle all logging messages
        /// </summary>
        public void Handle(DynamicMessageView<LogMessage> view, List<LogEvent> events)
        {
            // Enumerate all messages
            foreach (LogMessage message in view)
            {
                events.Add(new LogEvent
                {
                    Severity = (LogSeverity)message.severity,
                    Message = $"{ConnectionViewModel?.Application?.Name} - [{message.system.String}] {message.message.String}"
                });
            }
        }

        /// <summary>
        /// Handle an instrumentation report
        /// </summary>
        public void Handle(InstrumentationDiagnosticMessage message, List<LogEvent> events)
        {
            // Create bindable report
            InstrumentationReportViewModel report = new()
            {
                PassedShaders = (int)message.passedShaders,
                FailedShaders = (int)message.failedShaders,
                PassedPipelines = (int)message.passedPipelines,
                FailedPipelines = (int)message.failedPipelines,
                TotalMilliseconds = (int)message.millisecondsTotal,
                ShaderMilliseconds = (int)message.millisecondsShaders,
                PipelineMilliseconds = (int)message.millisecondsPipelines
            };

            // Get all compiler messages
            foreach (CompilationDiagnosticMessage compilerMessage in new DynamicMessageView<CompilationDiagnosticMessage>(message.messages.Stream))
            {
                report.Messages.Add(compilerMessage.content.String);
            }
            
            // Any failed?
            if (message.failedShaders + message.failedPipelines > 0)
            {
                events.Add(new LogEvent
                {
                    Severity = LogSeverity.Error,
                    Message = $"Instrumentation failed for {message.failedShaders} shaders and {message.failedPipelines} pipelines",
                    ViewModel = report
                });
            }
            
            // Report passed objects
            events.Add(new LogEvent
            {
                Severity = LogSeverity.Info,
                Message = $"Instrumented {message.passedShaders} shaders ({message.millisecondsShaders} ms) and {message.passedPipelines} pipelines ({message.millisecondsPipelines} ms), total {message.millisecondsTotal} ms",
                ViewModel = report
            });
        }

        /// <summary>
        /// Shared logging view model
        /// </summary>
        private ILoggingViewModel? _loggingViewModel = App.Locator.GetService<ILoggingService>()?.ViewModel;
    }
}