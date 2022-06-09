using System;
using Avalonia;
using Avalonia.Platform;
using Avalonia.Threading;

namespace Studio.Models
{
    public delegate void OnWorkspaceConnected(WorkspaceConnection sender);

    public delegate void OnWorkspaceRefused(WorkspaceConnection sender);

    public class WorkspaceConnection
    {
        // Invoked during connection
        public OnWorkspaceConnected Connected;

        // Invoked during connection rejection
        public OnWorkspaceRefused Refused;

        // Is this connection active?
        public bool IsConnected => _remote != null;

        public WorkspaceConnection()
        {
            // Create dispatcher local to the connection
            _dispatcher = new Dispatcher(AvaloniaLocator.Current.GetService<IPlatformThreadingInterface>());
        }

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
                    Connected?.Invoke(this);
                }
                else
                {
                    Refused?.Invoke(this);
                }
            }
        }

        public void ConnectAsync(string ipvx)
        {
            _dispatcher.InvokeAsync(() => Connect(ipvx));
        }

        public void DiscoverAsync()
        {
            _remote?.DiscoverAsync();
        }

        public Bridge.CLR.IBridge GetBridge()
        {
            lock (this)
            {
                return _remote;
            }
        }

        // Internal dispatcher
        private Dispatcher _dispatcher;

        // Underlying bridge
        private Bridge.CLR.RemoteClientBridge _remote;
    }
}