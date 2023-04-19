using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Windows.Input;
using Avalonia;
using Discovery.CLR;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Workspace;
using Studio.Services;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels
{
    public class LaunchViewModel : ReactiveObject
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
                if (WorkingDirectoryPath == string.Empty || WorkingDirectoryPath == System.IO.Path.GetDirectoryName(_applicationPath))
                {
                    WorkingDirectoryPath = System.IO.Path.GetDirectoryName(value)!;
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
        /// All virtual workspaces
        /// </summary>
        public ObservableCollection<Controls.IObservableTreeItem> Workspaces { get; } = new();

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
            WorkspaceViewModel = new WorkspaceViewModel()
            {
                Connection = new VirtualConnectionViewModel()
                {
                    Application = new ApplicationInfo()
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
                    featureCollectionViewModel.Features.Add(new FeatureInfo()
                    {
                        Name = instrumentationPropertyService.Name,
                        FeatureBit = (1u << index)
                    });
                    
                    // Internal mapping
                    _virtualFeatureMappings.Add(instrumentationPropertyService.Name, index);
                }
            }
            
            // Add workspace item
            Workspaces.Add(new Controls.WorkspaceTreeItemViewModel()
            {
                OwningContext = WorkspaceViewModel,
                ViewModel = WorkspaceViewModel.PropertyCollection
            });
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

            // Handle as accepted
            if (!await AcceptLaunch.Handle(Unit.Default))
            {
                return;
            }

            // Setup process info
            var processInfo = new DiscoveryProcessInfo();
            processInfo.applicationPath = _applicationPath;
            processInfo.workingDirectoryPath = _workingDirectoryPath;
            processInfo.arguments = _arguments;

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
        }

        /// <summary>
        /// Internal configurations
        /// </summary>
        private IEnumerable<IPropertyViewModel>? _selectedPropertyConfigurations;

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
        /// Internal virtual mappings
        /// </summary>
        Dictionary<string, int> _virtualFeatureMappings = new();
    }
}