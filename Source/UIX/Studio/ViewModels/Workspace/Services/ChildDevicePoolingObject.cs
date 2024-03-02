using System;
using System.Threading;
using Avalonia;
using Avalonia.Threading;
using Bridge.CLR;
using DynamicData;
using Message.CLR;
using Studio.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace.Services
{
    public class ChildDevicePoolingObject : IBridgeListener
    {
        public ChildDevicePoolingObject(IPropertyViewModel targetViewModel, ApplicationInfoViewModel applicationInfo)
        {
            _applicationInfo = applicationInfo;
            _targetViewModel = targetViewModel;
            
            // Create thread
            _thread = new Thread(CreateEndpointThread);

            // Create connection on main thread to ensure the right dispatcher is assigned
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                _connection = new();
                _thread.Start();
            });
        }

        /// <summary>
        /// Thread entry point for endpoint creation
        /// </summary>
        private void CreateEndpointThread(object? opaqueEntry)
        {
            // Once connected, immediately request the target application
            _connection.Connected.Subscribe(_ =>
            {
                _connection.Bridge?.Register(this);
                _connection.RequestClientAsync(_applicationInfo);
            });
            
            // Create connection
            _connection.Connect("127.0.0.1", null);
        }
        
        /// <summary>
        /// Invoked on bridge handlers
        /// </summary>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            foreach (OrderedMessage message in new OrderedMessageView(streams))
            {
                switch (message.ID)
                {
                    case HostConnectedMessage.ID:
                    {
                        Dispatcher.UIThread.InvokeAsync(OnHostConnected);
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Invoked on host connections
        /// </summary>
        private void OnHostConnected()
        {
            // Get provider
            var provider = App.Locator.GetService<IWorkspaceService>();

            // Create workspace
            var workspace = new WorkspaceViewModel()
            {
                Connection = _connection,
                OwningProperty = _targetViewModel
            };
                                
            // Configure and register workspace
            provider?.Install(workspace);

            // Add to target
            _targetViewModel.Properties.Add(workspace.PropertyCollection);
        }

        /// <summary>
        /// Owning thread
        /// </summary>
        private Thread _thread;

        /// <summary>
        /// Property to be added to
        /// </summary>
        private IPropertyViewModel _targetViewModel;

        /// <summary>
        /// Internal connection, fed to workspace
        /// </summary>
        private ConnectionViewModel _connection;

        /// <summary>
        /// Assigned application info
        /// </summary>
        private ApplicationInfoViewModel _applicationInfo;
    }
}