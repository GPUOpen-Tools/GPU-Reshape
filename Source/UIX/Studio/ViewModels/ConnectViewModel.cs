using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Reactive.Linq;
using System.Windows.Input;
using Avalonia;
using Avalonia.Threading;
using Bridge.CLR;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.ViewModels.Tools;

namespace Studio.ViewModels
{
    public class ConnectViewModel : ReactiveObject, IBridgeListener
    {
        /// <summary>
        /// Connect to selected application
        /// </summary>
        public ICommand Connect { get; }

        /// <summary>
        /// All resolved applications
        /// </summary>
        public ObservableCollection<Models.Workspace.ApplicationInfo> ResolvedApplications => _resolvedApplications;

        /// <summary>
        /// User interaction during connection acceptance
        /// </summary>
        public Interaction<ConnectViewModel, bool> AcceptClient { get; }
        
        /// <summary>
        /// Selected application for connection
        /// </summary>
        public Models.Workspace.ApplicationInfo? SelectedApplication
        {
            get => _selectedApplication;
            set => this.RaiseAndSetIfChanged(ref _selectedApplication, value);
        }

        /// <summary>
        /// Connection status
        /// </summary>
        public Models.Workspace.ConnectionStatus ConnectionStatus
        {
            get => _connectionStatus;
            set => this.RaiseAndSetIfChanged(ref _connectionStatus, value);
        }
        
        public ConnectViewModel()
        {
            // Initialize connection status
            ConnectionStatus = Models.Workspace.ConnectionStatus.None;

            // Create commands
            Connect = ReactiveCommand.Create(OnConnect);
            
            // Create interactions
            AcceptClient = new();
            
            // Subscribe
            _connectionViewModel.Connected.Subscribe(_ => OnRemoteConnected());
            
            // Start connection at localhost
            _connectionViewModel.ConnectAsync("127.0.0.1");
        }

        /// <summary>
        /// Connect implementation
        /// </summary>
        private void OnConnect()
        {
            if (SelectedApplication == null)
            {
                return;
            }
            
            // Set status
            ConnectionStatus = Models.Workspace.ConnectionStatus.Connecting;
            
            // Submit request
            _connectionViewModel.RequestClientAsync(SelectedApplication);
        }

        /// <summary>
        /// Invoked when the remote endpoint has connected
        /// </summary>
        private void OnRemoteConnected()
        {
            IBridge? bridge = _connectionViewModel.GetBridge();

            // Register handler
            bridge?.Register(this);
            
            // Request discovery
            _connectionViewModel.DiscoverAsync();
        }
        
        /// <summary>
        /// Message handler
        /// </summary>
        /// <param name="streams">all inbound streams</param>
        /// <param name="count">number of streams</param>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            // On main thread
            // TODO: This is ugly, ugly! Add some policy for threading or something
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                var schema = new OrderedMessageView(streams);

                // Visit typed
                foreach (OrderedMessage message in schema)
                {
                    switch (message.ID)
                    {
                        case HostDiscoveryMessage.ID:
                        {
                            Handle(message.Get<HostDiscoveryMessage>());
                            break;
                        }
                        case HostConnectedMessage.ID:
                        {
                            Handle(message.Get<HostConnectedMessage>());
                            break;
                        }
                        case HostResolvedMessage.ID:
                        {
                            Handle(message.Get<HostResolvedMessage>());
                            break;
                        }
                    }
                }
            });
        }

        /// <summary>
        /// Invoked on connection messages
        /// </summary>
        /// <param name="connected">message</param>
        private async void Handle(HostConnectedMessage connected)
        {
            // Set status
            if (connected.accepted == 0)
            {
                ConnectionStatus = Models.Workspace.ConnectionStatus.ApplicationRejected;
            }
            else
            {
                ConnectionStatus = Models.Workspace.ConnectionStatus.ApplicationAccepted;
            }
            
            // Confirm with view
            if (!await AcceptClient.Handle(this))
            {
                return;
            }
            
            // Get provider
            var provider = App.Locator.GetService<Services.IWorkspaceService>();

            // Create workspace
            provider?.Add(new ViewModels.Workspace.WorkspaceViewModel()
            {
                Connection = _connectionViewModel
            });
        }

        /// <summary>
        /// Invoked on resolved messages
        /// </summary>
        /// <param name="resolved">message</param>
        private async void Handle(HostResolvedMessage resolved)
        {
            // Set status
            if (resolved.accepted == 0)
            {
                ConnectionStatus = Models.Workspace.ConnectionStatus.ResolveRejected;
            }
            else
            {
                ConnectionStatus = Models.Workspace.ConnectionStatus.ResolveAccepted;
            }
        }

        /// <summary>
        /// Invoked on discovery messages
        /// </summary>
        /// <param name="discovery">message</param>
        private void Handle(HostDiscoveryMessage discovery)
        {
            var schema = new OrderedMessageView(discovery.infos.Stream);

            // Create new app list
            var apps = new List<Models.Workspace.ApplicationInfo>();

            // Visit typed
            foreach (OrderedMessage message in schema)
            {
                switch (message.ID)
                {
                    case HostServerInfoMessage.ID:
                    {
                        var info = message.Get<HostServerInfoMessage>();
                        
                        // Add application
                        apps.Add(new Models.Workspace.ApplicationInfo
                        {
                            Name = info.application.String,
                            Pid = info.processId,
                            Guid = new Guid(info.guid.String)
                        });
                        break;
                    }
                }
            }
            
            // Add on UI thread
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                // Set new app list
                _resolvedApplications.Clear();
                _resolvedApplications.Add(apps);
                
                // Set status
                ConnectionStatus = Models.Workspace.ConnectionStatus.None;
            });
        }

        /// <summary>
        /// Internal connection
        /// </summary>
        private Workspace.ConnectionViewModel _connectionViewModel = new();
        
        /// <summary>
        /// Internal connection status
        /// </summary>
        private Models.Workspace.ConnectionStatus _connectionStatus;

        /// <summary>
        /// Internal selected application
        /// </summary>
        private Models.Workspace.ApplicationInfo? _selectedApplication;
        
        /// <summary>
        /// Internal application list
        /// </summary>
        private ObservableCollection<Models.Workspace.ApplicationInfo> _resolvedApplications = new();
    }
}
