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