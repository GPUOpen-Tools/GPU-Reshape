﻿// 
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
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Diagnostic;
using Studio.Services;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Status
{
    public class InstrumentationStatusViewModel : ReactiveObject, IStatusViewModel, IBridgeListener
    {
        /// <summary>
        /// Standard orientation
        /// </summary>
        public StatusOrientation Orientation => StatusOrientation.Right;

        /// <summary>
        /// Current instrumentation stage
        /// </summary>
        public InstrumentationStage Stage
        {
            get => _stage;
            set => this.RaiseAndSetIfChanged(ref _stage, value);
        }

        /// <summary>
        /// Total number of graphics jobs
        /// </summary>
        public int GraphicsCount
        {
            get => _graphicsCount;
            set => this.RaiseAndSetIfChanged(ref _graphicsCount, value);
        }

        /// <summary>
        /// Total number of compute jobs
        /// </summary>
        public int ComputeCount
        {
            get => _computeCount;
            set => this.RaiseAndSetIfChanged(ref _computeCount, value);
        }
        
        /// <summary>
        /// Number of jobs in flight
        /// </summary>
        public int JobCount
        {
            get => _jobCount;
            set
            {
                this.RaiseAndSetIfChanged(ref _jobCount, value);
                this.RaisePropertyChanged(nameof(AnyJobs));
            }
        }

        /// <summary>
        /// Any jobs at the moment?
        /// </summary>
        public bool AnyJobs => _jobCount > 0;

        public InstrumentationStatusViewModel()
        {
            // Connect to workspaces
            ServiceRegistry.Get<IWorkspaceService>()?
                .WhenAnyValue(x => x.SelectedWorkspace)
                .Subscribe(OnWorkspaceChanged);
        }
        
        /// <summary>
        /// Invoked on workspace changes
        /// </summary>
        private void OnWorkspaceChanged(IWorkspaceViewModel? workspaceViewModel)
        {
            // Remove last connection
            _lastBridge?.Deregister(this);
            _lastBridge = null;
            
            // Clean state
            Stage         = InstrumentationStage.None;
            GraphicsCount = 0;
            ComputeCount  = 0;
            JobCount      = 0;

            // Register new one
            if (workspaceViewModel?.Connection?.Bridge is { } bridge)
            {
                workspaceViewModel.Connection?.Bridge?.Register(this);
                _lastBridge = bridge;
            }
        }

        /// <summary>
        /// Invoked on message handling
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.Schema.IsOrdered())
                return;
            
            foreach (OrderedMessage message in new OrderedMessageView(streams))
            {
                switch (message.ID)
                {
                    case JobDiagnosticMessage.ID:
                        OnMessage(message.Get<JobDiagnosticMessage>());
                        break;
                }
            }
        }
        
        /// <summary>
        /// Handle job diagnostic messages
        /// </summary>
        /// <param name="message"></param>
        private void OnMessage(JobDiagnosticMessage message)
        {
            // Flatten message
            var flat = message.Flat;
            
            // Schedule update
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                Stage         = (InstrumentationStage)flat.stage;
                GraphicsCount = (int)flat.graphicsJobs;
                ComputeCount  = (int)flat.computeJobs;
                JobCount      = (int)flat.remaining;
            });
        }

        /// <summary>
        /// Internal job counter
        /// </summary>
        private int _jobCount = 0;

        /// <summary>
        /// Internal stage
        /// </summary>
        private InstrumentationStage _stage = InstrumentationStage.None;

        /// <summary>
        /// The last connection we registered to
        /// </summary>
        private IBridge? _lastBridge;

        /// <summary>
        /// Internal graphics count
        /// </summary>
        private int _graphicsCount = 0;
        
        /// <summary>
        /// Internal compute count
        /// </summary>
        private int _computeCount = 0;
    }
}