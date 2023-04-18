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
    public class PipelineFilterViewModel : BasePropertyViewModel, IInstrumentableObject, IClosableObject, IBusObject
    {
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
            set => this.CheckRaiseAndSetIfChanged(ref _instrumentationState, value);
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public PipelineFilterViewModel() : base ("Filter ...", PropertyVisibility.WorkspaceTool)
        {
            CloseCommand = ReactiveCommand.Create(OnClose);

            // Connect property changes
            Properties
                .Connect()
                .OnItemAdded(OnPropertyChanged)
                .OnItemRemoved(OnPropertyChanged)
                .Subscribe();
        }

        /// <summary>
        /// Invoked on property changes
        /// </summary>
        /// <param name="obj"></param>
        private void OnPropertyChanged(IPropertyViewModel obj)
        {
            if (obj is IInstrumentationProperty)
            {
                this.EnqueueBus();
            }
        }

        /// <summary>
        /// Get the targetable property
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel? GetOrCreateInstrumentationProperty()
        {
            return this;
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
            if (bus != null)
            {
                // Submit request
                var request = bus.Add<RemoveFilteredPipelineInstrumentationMessage>(new RemoveFilteredPipelineInstrumentationMessage.AllocationInfo
                {
                    guidLength = (ulong)_guid.ToString().Length
                });
                request.guid.SetString(_guid.ToString());
            }
            
            // Track
            _instrumentationState = new();
            
            // Remove from parent
            this.DetachFromParent();
        }

        /// <summary>
        /// Commit all state
        /// </summary>
        /// <param name="stream"></param>
        public void Commit(OrderedMessageView<ReadWriteMessageStream> stream)
        {
            // Create new state
            InstrumentationState = new InstrumentationState();

            // Commit all child properties
            foreach (IInstrumentationProperty instrumentationProperty in this.GetProperties<IInstrumentationProperty>())
            {
                instrumentationProperty.Commit(InstrumentationState);
            }

            // Submit request
            var request = stream.Add<SetOrAddFilteredPipelineInstrumentationMessage>(new SetOrAddFilteredPipelineInstrumentationMessage.AllocationInfo
            {
                guidLength = (ulong)_guid.ToString().Length,
                nameLength = (ulong)(Filter.Name?.Length ?? 0)
            });
            request.guid.SetString(_guid.ToString());
            request.featureBitSet = InstrumentationState.FeatureBitMask;
            request.type = (uint)(Filter.Type ?? 0);
            request.name.SetString(Filter.Name ?? string.Empty);
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
        /// Internal query
        /// </summary>
        private PipelineFilterQueryViewModel _filter;
        
        /// <summary>
        /// Unique guid to this filter
        /// </summary>
        private Guid _guid = Guid.NewGuid();

        /// <summary>
        /// Tracked state
        /// </summary>
        private InstrumentationState _instrumentationState = new();
    }
}