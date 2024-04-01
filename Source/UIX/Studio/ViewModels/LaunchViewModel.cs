// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Environment;
using Studio.Models.Workspace;
using Studio.Services;
using Studio.Services.Suspension;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Config;

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
        [DataMember]
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
        /// Application working directory
        /// </summary>
        [DataMember]
        public string WorkingDirectoryPath
        {
            get => _workingDirectoryPath;
            set => this.RaiseAndSetIfChanged(ref _workingDirectoryPath, value);
        }

        /// <summary>
        /// Application launch arguments
        /// </summary>
        [DataMember]
        public string Arguments
        {
            get => _arguments;
            set => this.RaiseAndSetIfChanged(ref _arguments, value);
        }

        /// <summary>
        /// Application environment strings
        /// </summary>
        [DataMember]
        public string Environment
        {
            get => _environment;
            set => this.RaiseAndSetIfChanged(ref _environment, value);
        }

        /// <summary>
        /// Should the configuration safe guard?
        /// </summary>
        [DataMember]
        public bool CaptureChildProcesses
        {
            get => _captureChildProcesses;
            set => this.RaiseAndSetIfChanged(ref _captureChildProcesses, value);
        }

        /// <summary>
        /// Should the configuration safe guard?
        /// </summary>
        [DataMember]
        public bool AttachAllDevices
        {
            get => _attachAllDevices;
            set => this.RaiseAndSetIfChanged(ref _attachAllDevices, value);
        }

        /// <summary>
        /// The currently selected configuration
        /// </summary>
        public IWorkspaceConfigurationViewModel? SelectedConfiguration
        {
            get => _selectedConfiguration;
            set
            {
                this.RaiseAndSetIfChanged(ref _selectedConfiguration, value);
                OnSelectionChanged();
            }
        }

        /// <summary>
        /// The description of the selected configuration
        /// </summary>
        public string SelectedConfigurationDescription
        {
            get => _selectedConfigurationDescription;
            set => this.RaiseAndSetIfChanged(ref _selectedConfigurationDescription, value);
        }

        /// <summary>
        /// Should the configuration report detailed errors?
        /// </summary>
        [DataMember]
        public bool Detail
        {
            get => _detail;
            set
            {
                this.RaiseAndSetIfChanged(ref _detail, value);
                OnDetailChanged();
            }
        }

        /// <summary>
        /// Should the configuration safe guard?
        /// </summary>
        [DataMember]
        public bool SafeGuard
        {
            get => _safeGuard;
            set
            {
                this.RaiseAndSetIfChanged(ref _safeGuard, value);
                OnSafeGuardChanged();
            }
        }

        /// <summary>
        /// Should the configuration synchronously record?
        /// </summary>
        [DataMember]
        public bool SynchronousRecording
        {
            get => _synchronousRecording;
            set
            {
                this.RaiseAndSetIfChanged(ref _synchronousRecording, value);
                OnSynchronousRecordingChanged();
            }
        }

        /// <summary>
        /// Name of the configuration, kept for suspension purposes
        /// </summary>
        [DataMember]
        public string SelectedConfigurationName
        {
            get => _selectedConfigurationName;
            set => this.RaiseAndSetIfChanged(ref _selectedConfigurationName, value);
        }

        /// <summary>
        /// All configurations
        /// </summary>
        public ObservableCollection<IWorkspaceConfigurationViewModel> Configurations { get; } = new();

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

            // Create the initial workspace
            CreateVirtualWorkspace();

            // Subscribe
            _connectionViewModel.Connected.Subscribe(_ => OnRemoteConnected());
            
            // Bind configurations
            App.Locator.GetService<IWorkspaceService>()?.Configurations.Connect()
                .OnItemAdded(x => Configurations.Add(x))
                .OnItemRemoved(x => Configurations.Remove(x))
                .Subscribe();

            // Suspension
            this.BindTypedSuspension();

            // Try getting configuration from suspension
            if (!string.IsNullOrEmpty(SelectedConfigurationName))
            {
                SelectedConfiguration = Configurations.FirstOrDefault(x => x.Name == SelectedConfigurationName);
            }
            
            // Default selection
            if (SelectedConfiguration == null)
            {
                SelectedConfiguration = Configurations[0];
            }
        }

        /// <summary>
        /// Create a new virtual workspace
        /// </summary>
        private void CreateVirtualWorkspace()
        {
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
                    Application = new ApplicationInfoViewModel
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
            
            // Remove old mappings
            _virtualFeatureMappings.Clear();
            
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
            
            // Remove old workspaces
            Workspaces.Clear();
            
            // Add workspace item
            Workspaces.Add(new WorkspaceTreeItemViewModel
            {
                OwningContext = WorkspaceViewModel,
                ViewModel = WorkspaceViewModel.PropertyCollection
            });
        }

        /// <summary>
        /// Invoked on selection changes
        /// </summary>
        private void OnSelectionChanged()
        {
            if (SelectedConfiguration == null)
            {
                return;
            }
            
            // Create new workspace
            CreateVirtualWorkspace();
            
            // Install on new workspace
            SelectedConfiguration.Install(WorkspaceViewModel);

            // Force recording state?
            if (SelectedConfiguration.Flags.HasFlag(WorkspaceConfigurationFlag.RequiresSynchronousRecording))
            {
                SynchronousRecording = true;
            }
            
            // Re-apply properties
            OnDetailChanged();
            OnSafeGuardChanged();
            OnSynchronousRecordingChanged();

            // Get new description
            SelectedConfigurationDescription = SelectedConfiguration.GetDescription(WorkspaceViewModel);
            
            // Set name for suspension
            SelectedConfigurationName = SelectedConfiguration.Name;
        }

        /// <summary>
        /// Invoked on safe guarding changes
        /// </summary>
        private void OnSafeGuardChanged()
        {
            if ((WorkspaceViewModel.PropertyCollection as IInstrumentableObject)?.GetOrCreateInstrumentationProperty()?.GetProperty<InstrumentationConfigViewModel>() is {} config)
            {
                config.SafeGuard = SafeGuard;
            }
        }

        /// <summary>
        /// Invoked on detailed changes
        /// </summary>
        private void OnDetailChanged()
        {
            if ((WorkspaceViewModel.PropertyCollection as IInstrumentableObject)?.GetOrCreateInstrumentationProperty()?.GetProperty<InstrumentationConfigViewModel>() is {} config)
            {
                config.Detail = Detail;
            }
        }

        /// <summary>
        /// Invoked on synchronous recording changes
        /// </summary>
        private void OnSynchronousRecordingChanged()
        {
            if (WorkspaceViewModel.PropertyCollection.GetProperty<ApplicationInstrumentationConfigViewModel>() is {} config)
            {
                config.SynchronousRecording = SynchronousRecording;
            }
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
            var processInfo = new DiscoveryProcessCreateInfo();
            processInfo.applicationPath = _applicationPath;
            processInfo.workingDirectoryPath = _workingDirectoryPath;
            processInfo.arguments = _arguments;
            processInfo.reservedToken = _pendingReservedToken;
            processInfo.captureChildProcesses = _captureChildProcesses;
            processInfo.attachAllDevices = _attachAllDevices;

            // Parse environment
            processInfo.environment = new(EnvironmentParser.Parse(_environment).Select(kv => Tuple.Create(kv.Key, kv.Value)));

            // If no working directory has been specified, default to the executable
            if (string.IsNullOrWhiteSpace(processInfo.workingDirectoryPath))
            {
                processInfo.workingDirectoryPath = Path.GetDirectoryName(processInfo.applicationPath);
            }

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
                busPropertyService.CommitRedirect(view, false);
            }
            
            // Start process
            service.StartBootstrappedProcess(processInfo, view.Storage, ref _discoveryProcessInfo);
            
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
            if (AttachAllDevices)
            {
                Dispatcher.UIThread.InvokeAsync(CreateProcessWorkspace);
                return;
            }
            
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
        /// Create a process workspace
        /// </summary>
        private async void CreateProcessWorkspace()
        {
            // Get provider
            var provider = App.Locator.GetService<IWorkspaceService>();

            // Create process workspace
            var workspace = new ProcessWorkspaceViewModel(_pendingReservedToken)
            {
                Connection = _connectionViewModel
            };

            // Assume application info from paths
            _connectionViewModel.Application = new ApplicationInfoViewModel()
            {
                Guid = Guid.NewGuid(),
                Pid = _discoveryProcessInfo.processId,
                Process = Path.GetFileName(_applicationPath),
                DecorationMode = ApplicationDecorationMode.ProcessOnly
            };

            // Configure and register workspace
            provider?.Install(workspace);
            provider?.Add(workspace, false);

            // Handle as accepted
            await AcceptLaunch.Handle(Unit.Default);
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
                provider?.Add(workspace, true);
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
            // Ignore if unassigned?
            if (string.IsNullOrEmpty(_pendingReservedToken))
            {
                return;
            }
            
            // Assigned to reserved application
            ApplicationInfoViewModel? applicationInfo = null;

            // Visit typed
            foreach (OrderedMessage message in new OrderedMessageView(discovery.infos.Stream))
            {
                switch (message.ID)
                {
                    case HostServerInfoMessage.ID:
                    {
                        var info = message.Get<HostServerInfoMessage>();
                        
                        // Matched against pending?
                        if (info.reservedGuid.String.Equals(_pendingReservedToken, StringComparison.InvariantCultureIgnoreCase))
                        {
                            applicationInfo = new ApplicationInfoViewModel
                            {
                                Name = info.application.String,
                                Process = info.process.String,
                                API = info.api.String,
                                Pid = info.processId,
                                DeviceUid = info.deviceUid,
                                DeviceObjects = info.deviceObjects,
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
        /// Internal environment
        /// </summary>
        private string _environment = string.Empty;

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

        /// <summary>
        /// Internal selected configuration state
        /// </summary>
        private IWorkspaceConfigurationViewModel? _selectedConfiguration;
        
        /// <summary>
        /// Internal configuration description state
        /// </summary>
        private string _selectedConfigurationDescription;
        
        /// <summary>
        /// Internal capture child processes state
        /// </summary>
        private bool _captureChildProcesses;
        
        /// <summary>
        /// Internal attach all devices state
        /// </summary>
        private bool _attachAllDevices;
        
        /// <summary>
        /// Internal safe guard state
        /// </summary>
        private bool _safeGuard;

        /// <summary>
        /// Internal detail state
        /// </summary>
        private bool _detail;
        
        /// <summary>
        /// Internal synchronous recording state
        /// </summary>
        private bool _synchronousRecording;
        
        /// <summary>
        /// Internal name state
        /// </summary>
        private string _selectedConfigurationName;

        /// <summary>
        /// Instantiated process info
        /// </summary>
        private DiscoveryProcessInfo _discoveryProcessInfo = new();
    }
}