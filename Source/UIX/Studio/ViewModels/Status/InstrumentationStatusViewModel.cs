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
    public class InstrumentationStatusViewModel : ReactiveObject, IStatusViewModel, IBridgeListener
    {
        /// <summary>
        /// Standard orientation
        /// </summary>
        public StatusOrientation Orientation => StatusOrientation.Right;

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
            AvaloniaLocator.Current.GetService<IWorkspaceService>()?.Workspaces .Connect()
                .OnItemAdded(OnWorkspaceAdded)
                .OnItemRemoved(OnWorkspaceRemoved)
                .Subscribe();
        }

        /// <summary>
        /// Invoked whena  workspace has been added
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        private void OnWorkspaceAdded(IWorkspaceViewModel workspaceViewModel)
        {
            workspaceViewModel.Connection?.Bridge?.Register(this);
        }
        
        /// <summary>
        /// Invoked when a workspace has been removed
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        private void OnWorkspaceRemoved(IWorkspaceViewModel workspaceViewModel)
        {
            workspaceViewModel.Connection?.Bridge?.Deregister(this);
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
            int count = (int)message.remaining;
            Dispatcher.UIThread.InvokeAsync(() => JobCount = count);
        }

        /// <summary>
        /// Internal job counter
        /// </summary>
        private int _jobCount = 0;
    }
}