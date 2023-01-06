using System;
using System.Diagnostics;
using System.Reactive;
using System.Reactive.Subjects;
using Avalonia;
using Avalonia.Controls.Notifications;
using Avalonia.Platform;
using Avalonia.Threading;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Workspace;

namespace Studio.ViewModels.Workspace
{
    public interface IConnectionViewModel
    {
        /// <summary>
        /// Associated application information
        /// </summary>
        public ApplicationInfo? Application { get; }
        
        /// <summary>
        /// Invoked during connection
        /// </summary>
        public ISubject<Unit> Connected { get; }
        
        /// <summary>
        /// Invoked during connection rejection
        /// </summary>
        public ISubject<Unit> Refused { get; }
        
        /// <summary>
        /// Bridge within this connection
        /// </summary>
        public Bridge.CLR.IBridge? Bridge { get; }

        /// <summary>
        /// Time on the remote endpoint
        /// </summary>
        public DateTime LocalTime { get; set; }
        
        /// <summary>
        /// Get the shared bus
        /// </summary>
        /// <returns></returns>
        public OrderedMessageView<ReadWriteMessageStream> GetSharedBus();

        /// <summary>
        /// Commit all pending messages
        /// </summary>
        public void Commit();
    }
}