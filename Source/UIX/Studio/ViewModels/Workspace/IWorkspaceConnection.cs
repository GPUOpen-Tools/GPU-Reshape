using System;
using System.Diagnostics;
using System.Reactive;
using System.Reactive.Subjects;
using Avalonia;
using Avalonia.Controls.Notifications;
using Avalonia.Platform;
using Avalonia.Threading;
using ReactiveUI;

namespace Studio.ViewModels.Workspace
{
    public interface IWorkspaceConnection
    {
        /// <summary>
        /// Invoked during connection
        /// </summary>
        ISubject<Unit> Connected { get; }
        
        /// <summary>
        /// Invoked during connection rejection
        /// </summary>
        ISubject<Unit> Refused { get; }
    }
}