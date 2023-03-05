using System;
using System.Windows.Input;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Listeners;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace Studio.ViewModels.Workspace.Properties
{
    public class WorkspaceCollectionViewModel : ReactiveObject, IPropertyViewModel, IInstrumentableObject, IClosableObject, IDescriptorObject
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name
        {
            get => ConnectionViewModel?.Application?.DecoratedName ?? "Invalid";
        }

        /// <summary>
        /// Visibility of this property
        /// </summary>
        public PropertyVisibility Visibility => PropertyVisibility.WorkspaceTool;

        /// <summary>
        /// Parent property
        /// </summary>
        public IPropertyViewModel? Parent { get; set; }

        /// <summary>
        /// Child properties
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; set; } = new SourceList<IPropertyViewModel>();

        /// <summary>
        /// All services
        /// </summary>
        public ISourceList<IPropertyService> Services { get; set; } = new SourceList<IPropertyService>();

        /// <summary>
        /// Document descriptor
        /// </summary>
        public IDescriptor? Descriptor { get; set; }

        /// <summary>
        /// Close command
        /// </summary>
        public ICommand? CloseCommand { get; set; }

        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _connectionViewModel, value);

                OnConnectionChanged();
            }
        }

        /// <summary>
        /// Instrumentation handler
        /// </summary>
        public InstrumentationState InstrumentationState
        {
            get
            {
                return this.GetProperty<GlobalViewModel>()?.InstrumentationState ?? new InstrumentationState();
            }
            set
            {
                // Get global instrumentation
                var globalViewModel = this.GetProperty<GlobalViewModel>();
                if (globalViewModel == null)
                {
                    Properties.Add(globalViewModel = new GlobalViewModel()
                    {
                        Parent = this,
                        ConnectionViewModel = ConnectionViewModel
                    });
                }

                // Pass down
                globalViewModel.InstrumentationState = value;
            }
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            CreateProperties();
            CreateServices();
        }

        /// <summary>
        /// Create all services
        /// </summary>
        private void CreateServices()
        {
            // Create pulse
            var pulseService = new PulseService()
            {
                ConnectionViewModel = ConnectionViewModel
            };

            // Bind to local time
            pulseService.WhenAnyValue(x => x.LastPulseTime).Subscribe(x => ConnectionViewModel!.LocalTime = x);
            
            // Register pulse service
            Services.Add(pulseService);
        }

        /// <summary>
        /// Create all properties
        /// </summary>
        private void CreateProperties()
        {
            Properties.Clear();

            if (_connectionViewModel == null)
            {
                return;
            }
            
            Properties.AddRange(new IPropertyViewModel[]
            {
                new LogViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                },
                
                new FeatureCollectionViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                },
                
                new MessageCollectionViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                },
                
                new ShaderCollectionViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                },

                new PipelineCollectionViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = _connectionViewModel
                }
            });
        }

        /// <summary>
        /// Get the workspace
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel GetWorkspace()
        {
            return this;
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}