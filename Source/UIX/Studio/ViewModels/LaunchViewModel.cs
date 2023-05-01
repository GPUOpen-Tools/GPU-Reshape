﻿using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Windows.Input;
using Avalonia;
using Avalonia.Threading;
using Bridge.CLR;
using Discovery.CLR;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Workspace;
using Studio.Services;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels
{
    public class LaunchViewModel : ReactiveObject, IBridgeListener
    {
        /// <summary>
        /// Connect to selected application
        /// </summary>
        public ICommand Start { get; }

        /// <summary>
        /// Current connection string
        /// </summary>
        public string ApplicationPath
        {
            get => _applicationPath;
            set
            {
                if (WorkingDirectoryPath == string.Empty || WorkingDirectoryPath == Path.GetDirectoryName(_applicationPath))
                {
                    WorkingDirectoryPath = Path.GetDirectoryName(value)!;
                }

                this.RaiseAndSetIfChanged(ref _applicationPath, value);
            }
        }

        /// <summary>
        /// Current connection string
        /// </summary>
        public string WorkingDirectoryPath
        {
            get => _workingDirectoryPath;
            set => this.RaiseAndSetIfChanged(ref _workingDirectoryPath, value);
        }

        /// <summary>
        /// Current connection string
        /// </summary>
        public string Arguments
        {
            get => _arguments;
            set => this.RaiseAndSetIfChanged(ref _arguments, value);
        }

        /// <summary>
        /// Connection status
        /// </summary>
        public ConnectionStatus ConnectionStatus
        {
            get => _connectionStatus;
            set => this.RaiseAndSetIfChanged(ref _connectionStatus, value);
        }

        /// <summary>
        /// All virtual workspaces
        /// </summary>
        public ObservableCollection<IObservableTreeItem> Workspaces { get; } = new();

        /// <summary>
        /// Current selected property
        /// </summary>
        public IPropertyViewModel? SelectedProperty
        {
            get => _selectedProperty;
            set => this.RaiseAndSetIfChanged(ref _selectedProperty, value);
        }
        
        /// <summary>
        /// Virtual workspace
        /// </summary>
        public WorkspaceViewModel WorkspaceViewModel;

        /// <summary>
        /// All configurations
        /// </summary>
        public IEnumerable<IPropertyViewModel>? SelectedPropertyConfigurations
        {
            get => _selectedPropertyConfigurations;
            set => this.RaiseAndSetIfChanged(ref _selectedPropertyConfigurations, value);
        }
        
        /// <summary>
        /// User interaction during launch acceptance
        /// </summary>
        public Interaction<Unit, bool> AcceptLaunch { get; }

        /// <summary>
        /// Can a launch be performed now?
        /// </summary>
        public bool CanLaunch
        {
            get => _canLaunch;
            set => this.RaiseAndSetIfChanged(ref _canLaunch, value);
        }
        
        /// <summary>
        /// Constructor
        /// </summary>
        public LaunchViewModel()
        {
            // Create commands
            Start = ReactiveCommand.Create(OnStart);

            // Create interactions
            AcceptLaunch = new();

            // Bind to selected property
            this.WhenAnyValue(x => x.SelectedProperty)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Filter by configurations
                    SelectedPropertyConfigurations = x.Properties.Items.Where(p => p.Visibility == PropertyVisibility.Configuration);
                });

            // Must have service
            if (App.Locator.GetService<IWorkspaceService>() is not { } service)
            {
                return;
            }

            // Create workspace with virtual adapter
            WorkspaceViewModel = new WorkspaceViewModel
            {
                Connection = new VirtualConnectionViewModel
                {
                    Application = new ApplicationInfo
                    {
                        Process = "Application"
                    }
                }
            };
            
            // Record all bus objects instead of directly committing them
            if (WorkspaceViewModel.PropertyCollection.GetService<IBusPropertyService>() is { } busPropertyService)
            {
                busPropertyService.Mode = BusMode.RecordAndCommit;
            }
            
            // Install standard extensions
            service.Install(WorkspaceViewModel);
            
            // Create virtualized feature space
            if (WorkspaceViewModel.PropertyCollection.GetProperty<FeatureCollectionViewModel>() is {} featureCollectionViewModel)
            {
                // Install all features
                foreach (IInstrumentationPropertyService instrumentationPropertyService in WorkspaceViewModel.PropertyCollection.GetServices<IInstrumentationPropertyService>())
                {
                    int index = _virtualFeatureMappings.Count;
                    
                    // Add feature
                    featureCollectionViewModel.Features.Add(new FeatureInfo
                    {
                        Name = instrumentationPropertyService.Name,
                        FeatureBit = (1u << index)
                    });
                    
                    // Internal mapping
                    _virtualFeatureMappings.Add(instrumentationPropertyService.Name, index);
                }
            }
            
            // Add workspace item
            Workspaces.Add(new WorkspaceTreeItemViewModel
            {
                OwningContext = WorkspaceViewModel,
                ViewModel = WorkspaceViewModel.PropertyCollection
            });

            // Subscribe
            _connectionViewModel.Connected.Subscribe(_ => OnRemoteConnected());
        }

        /// <summary>
        /// Invoked on start
        /// </summary>
        private async void OnStart()
        {
            if (App.Locator.GetService<IBackendDiscoveryService>()?.Service is not { } service)
            {
                return;
            }

            // Prevent launches for now
            _canLaunch = false;

            // Allocate token up-front
            _pendingReservedToken = $"{{{Guid.NewGuid()}}}";

            // Setup process info
            var processInfo = new DiscoveryProcessInfo();
            processInfo.applicationPath = _applicationPath;
            processInfo.workingDirectoryPath = _workingDirectoryPath;
            processInfo.arguments = _arguments;
            processInfo.reservedToken = _pendingReservedToken;

            // Create environment view
            var view = new OrderedMessageView<ReadWriteMessageStream>(new ReadWriteMessageStream());
            
            // Construct virtual redirects
            foreach ((string? key, int value) in _virtualFeatureMappings)
            {
                SetVirtualFeatureRedirectMessage message = view.Add<SetVirtualFeatureRedirectMessage>(new SetVirtualFeatureRedirectMessage.AllocationInfo
                {
                    nameLength = (ulong)key.Length
                });

                // Write redirect contents
                message.name.SetString(key);
                message.index = value;
            }

            // Commit all pending objects
            if (WorkspaceViewModel.PropertyCollection.GetService<IBusPropertyService>() is { } busPropertyService)
            {
                busPropertyService.CommitRedirect(view);
            }
            
            // Start process
            service.StartBootstrappedProcess(processInfo, view.Storage);
            
            // Start connection
            _connectionViewModel.Connect("127.0.0.1", null);
            
            // Update status
            ConnectionStatus = ConnectionStatus.Connecting;
        }

        /// <summary>
        /// Invoked when the remote endpoint has connected
        /// </summary>
        private void OnRemoteConnected()
        {
            // Register handler
            _connectionViewModel.Bridge?.Register(this);

            // Request discovery
            _connectionViewModel.DiscoverAsync();

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                // Existing?
                _timer?.Stop();

                // Create timer on main thread
                _timer = new DispatcherTimer(DispatcherPriority.Background)
                {
                    Interval = TimeSpan.FromSeconds(1),
                    IsEnabled = true
                };

                // Subscribe tick
                _timer.Tick += OnPoolingTick;

                // Must call start manually (a little vague)
                _timer.Start();
            });
        }

        /// <summary>
        /// Invoked on timer pooling
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnPoolingTick(object? sender, EventArgs e)
        {
            _connectionViewModel.DiscoverAsync();
        }

        /// <summary>
        /// Invoked on bridge handlers
        /// </summary>
        /// <param name="streams">all streams</param>
        /// <param name="count">number of streams</param>
        public void Handle(ReadOnlyMessageStream streams, uint count)
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
        }

        /// <summary>
        /// Invoked on connection messages
        /// </summary>
        /// <param name="connected">message</param>
        private void Handle(HostConnectedMessage connected)
        {
            var flat = connected.Flat;

            Dispatcher.UIThread.InvokeAsync(async () =>
            {
                // Set status
                if (flat.accepted == 0)
                {
                    ConnectionStatus = Models.Workspace.ConnectionStatus.ApplicationRejected;
                }
                else
                {
                    ConnectionStatus = Models.Workspace.ConnectionStatus.ApplicationAccepted;
                }

                // Handle as accepted
                if (!await AcceptLaunch.Handle(Unit.Default))
                {
                    return;
                }

                // Get provider
                var provider = App.Locator.GetService<Services.IWorkspaceService>();

                // Create workspace
                var workspace = new ViewModels.Workspace.WorkspaceViewModel()
                {
                    Connection = _connectionViewModel
                };
                
                // Configure and register workspace
                provider?.Install(workspace);
                provider?.Add(workspace);
            });
        }

        /// <summary>
        /// Invoked on resolved messages
        /// </summary>
        /// <param name="resolved">message</param>
        private void Handle(HostResolvedMessage resolved)
        {
            var flat = resolved.Flat;

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                // Set status
                if (flat.accepted == 0)
                {
                    ConnectionStatus = Models.Workspace.ConnectionStatus.ResolveRejected;
                }
                else
                {
                    ConnectionStatus = Models.Workspace.ConnectionStatus.ResolveAccepted;
                }
            });
        }

        /// <summary>
        /// Invoked on discovery messages
        /// </summary>
        /// <param name="discovery">message</param>
        private void Handle(HostDiscoveryMessage discovery)
        {
            // Assigned to reserved application
            ApplicationInfo? applicationInfo = null;

            // Visit typed
            foreach (OrderedMessage message in new OrderedMessageView(discovery.infos.Stream))
            {
                switch (message.ID)
                {
                    case HostServerInfoMessage.ID:
                    {
                        var info = message.Get<HostServerInfoMessage>();
                        
                        // Matched against pending?
                        if (info.guid.String.Equals(_pendingReservedToken, StringComparison.InvariantCultureIgnoreCase))
                        {
                            applicationInfo = new Models.Workspace.ApplicationInfo
                            {
                                Name = info.application.String,
                                Process = info.process.String,
                                API = info.api.String,
                                Pid = info.processId,
                                Guid = new Guid(info.guid.String)
                            };
                        }
                        break;
                    }
                }
            }
            
            // Mark as connected
            Dispatcher.UIThread.InvokeAsync(() => ConnectionStatus = ConnectionStatus.EndpointConnected);

            // Not found?
            if (applicationInfo == null)
            {
                return;
            }
            
            // Add on UI thread
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                // Prevent future timings
                _timer.Stop();
                
                // Submit request
                _connectionViewModel.RequestClientAsync(applicationInfo);
            });
        }

        /// <summary>
        /// Internal configurations
        /// </summary>
        private IEnumerable<IPropertyViewModel>? _selectedPropertyConfigurations;

        /// <summary>
        /// Internal connection
        /// </summary>
        private Workspace.ConnectionViewModel _connectionViewModel = new();

        /// <summary>
        /// Internal selected property
        /// </summary>
        private IPropertyViewModel? _selectedProperty;

        /// <summary>
        /// Internal application path
        /// </summary>
        private string _applicationPath = string.Empty;

        /// <summary>
        /// Internal working directory path
        /// </summary>
        private string _workingDirectoryPath = string.Empty;

        /// <summary>
        /// Internal arguments
        /// </summary>
        private string _arguments = string.Empty;

        /// <summary>
        /// Pending token for discovery matching
        /// </summary>
        private string _pendingReservedToken;

        /// <summary>
        /// Internal connection status
        /// </summary>
        private ConnectionStatus _connectionStatus;

        /// <summary>
        /// Shared bus timer
        /// </summary>
        private DispatcherTimer _timer;

        /// <summary>
        /// Internal launch state
        /// </summary>
        private bool _canLaunch = true;
        
        /// <summary>
        /// Internal virtual mappings
        /// </summary>
        Dictionary<string, int> _virtualFeatureMappings = new();
    }
}