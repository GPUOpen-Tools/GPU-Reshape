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
    public delegate void OnWorkspaceConnected(WorkspaceConnection sender);

    public delegate void OnWorkspaceRefused(WorkspaceConnection sender);

    public class WorkspaceConnection : ReactiveObject, IWorkspaceConnection
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
        /// Is the connection alive?
        /// </summary>
        public bool IsConnected => _remote != null;

        /// <summary>
        /// Endpoint application info
        /// </summary>
        public Models.Workspace.ApplicationInfo? Application { get; private set; }

        public WorkspaceConnection()
        {
            // Create subjects
            Connected = new Subject<Unit>();
            Refused = new Subject<Unit>();
            
            // Create dispatcher local to the connection
            _dispatcher = new Dispatcher(AvaloniaLocator.Current.GetService<IPlatformThreadingInterface>());
        }

        /// <summary>
        /// Connect synchronously
        /// </summary>
        /// <param name="ipvx">remote endpoint</param>
        public void Connect(string ipvx)
        {
            lock (this)
            {
                // Create bridge
                _remote = new Bridge.CLR.RemoteClientBridge();
                
                // Always commit when remote append streams
                _remote.SetCommitOnAppend(true);

                // Install against endpoint
                bool state = _remote.Install(new Bridge.CLR.EndpointResolve()
                {
                    config = new Bridge.CLR.EndpointConfig()
                    {
                        applicationName = "Studio"
                    },
                    ipvxAddress = ipvx
                });

                // Invoke handlers
                if (state)
                {
                    Connected.OnNext(Unit.Default);
                }
                else
                {
                    Refused.OnNext(Unit.Default);
                }
            }
        }

        /// <summary>
        /// Connect asynchronously
        /// </summary>
        /// <param name="ipvx">remote endpoint</param>
        public void ConnectAsync(string ipvx)
        {
            _dispatcher.InvokeAsync(() => Connect(ipvx));
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
        public void RequestClientAsync(ApplicationInfo applicationInfo)
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
        /// Get the active bridge
        /// </summary>
        /// <returns>base bridge interface</returns>
        public Bridge.CLR.IBridge? GetBridge()
        {
            lock (this)
            {
                return _remote;
            }
        }

        /// <summary>
        /// Internal dispatcher
        /// </summary>
        private Dispatcher _dispatcher;

        /// <summary>
        /// Internal underlying bridge
        /// </summary>
        private Bridge.CLR.RemoteClientBridge? _remote;
    }
}