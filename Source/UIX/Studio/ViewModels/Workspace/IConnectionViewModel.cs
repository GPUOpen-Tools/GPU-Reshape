using System;
using System.Diagnostics;
using System.Reactive;
using System.Reactive.Subjects;
using Avalonia;
using Avalonia.Controls.Notifications;
using Avalonia.Platform;
using Avalonia.Threading;
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
        ISubject<Unit> Connected { get; }
        
        /// <summary>
        /// Invoked during connection rejection
        /// </summary>
        ISubject<Unit> Refused { get; }
    }
}