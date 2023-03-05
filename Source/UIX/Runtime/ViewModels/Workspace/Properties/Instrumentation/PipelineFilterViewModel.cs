using System;
using System.Windows.Input;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Query;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Properties.Instrumentation
{
    public class PipelineFilterViewModel : ReactiveObject, IPropertyViewModel, IInstrumentableObject, IClosableObject
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name
        {
            get => _name;
            set => this.RaiseAndSetIfChanged(ref _name, value);
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
        /// Invoked on closes
        /// </summary>
        public ICommand? CloseCommand { get; set; }

        /// <summary>
        /// Constructed pipeline filter
        /// </summary>
        public PipelineFilterQueryViewModel Filter
        {
            get => _filter;
            set
            {
                this.RaiseAndSetIfChanged(ref _filter, value);
                OnFilterChanged();
            }
        }

        /// <summary>
        /// Current shader
        /// </summary>
        public PipelineIdentifier Pipeline
        {
            get => _pipeline;
            set
            {
                this.RaiseAndSetIfChanged(ref _pipeline, value);
                OnPipelineChanged();
            }
        }

        /// <summary>
        /// Instrumentation handler
        /// </summary>
        public InstrumentationState InstrumentationState
        {
            get => _instrumentationState;
            set
            {
                if (!this.CheckRaiseAndSetIfChanged(ref _instrumentationState, value))
                {
                    return;
                }
                
                // Get bus
                var bus = ConnectionViewModel?.GetSharedBus();
                if (bus == null)
                {
                    return;
                }

                // Submit request
                var request = bus.Add<SetOrAddFilteredPipelineInstrumentationMessage>(new SetOrAddFilteredPipelineInstrumentationMessage.AllocationInfo
                {
                    guidLength = (ulong)_guid.ToString().Length,
                    nameLength = (ulong)(Filter.Name?.Length ?? 0)
                });
                request.guid.SetString(_guid.ToString());
                request.featureBitSet = value.FeatureBitMask;
                request.type = (uint)(Filter.Type ?? 0);
                request.name.SetString(Filter.Name ?? string.Empty);
            }
        }

        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set => this.RaiseAndSetIfChanged(ref _connectionViewModel, value);
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public PipelineFilterViewModel()
        {
            CloseCommand = ReactiveCommand.Create(OnClose);
        }

        /// <summary>
        /// Invoked on filter changes
        /// </summary>
        private void OnFilterChanged()
        {
            Name = Filter.Decorator;
        }

        /// <summary>
        /// Invoked on close
        /// </summary>
        private void OnClose()
        {
            // Get bus
            var bus = ConnectionViewModel?.GetSharedBus();
            if (bus == null)
            {
                return;
            }

            // Submit request
            var request = bus.Add<RemoveFilteredPipelineInstrumentationMessage>(new RemoveFilteredPipelineInstrumentationMessage.AllocationInfo
            {
                guidLength = (ulong)_guid.ToString().Length
            });
            request.guid.SetString(_guid.ToString());
            
            // Track
            _instrumentationState = new();
            
            // Remove from parent
            this.DetachFromParent();
        }

        /// <summary>
        /// Invoked on pipeline changes
        /// </summary>
        private void OnPipelineChanged()
        {
            Name = Pipeline.Descriptor;
        }

        /// <summary>
        /// Get the owning workspace
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel? GetWorkspace()
        {
            return Parent?.GetRoot();
        }

        /// <summary>
        /// Code listener
        /// </summary>
        private PipelineIdentifier _pipeline;

        /// <summary>
        /// Internal name
        /// </summary>
        private string _name = "Filter ...";

        /// <summary>
        /// Internal query
        /// </summary>
        private PipelineFilterQueryViewModel _filter;
        
        /// <summary>
        /// Unique guid to this filter
        /// </summary>
        private Guid _guid = Guid.NewGuid();

        /// <summary>
        /// Internal connection
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Tracked state
        /// </summary>
        private InstrumentationState _instrumentationState = new();
    }
}