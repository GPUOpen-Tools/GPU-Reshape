using System;
using System.Windows.Input;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Properties.Instrumentation
{
    public class PipelineViewModel : BasePropertyViewModel, IInstrumentableObject, IClosableObject, IBusObject
    {
        /// <summary>
        /// Invoked on closes
        /// </summary>
        public ICommand? CloseCommand { get; set; }

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
        public PipelineViewModel() : base("Pipeline", PropertyVisibility.WorkspaceTool)
        {
            CloseCommand = ReactiveCommand.Create(OnClose);

            // Bind property changes
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
        /// Invoked on close
        /// </summary>
        private void OnClose()
        {
            // Get bus
            var bus = ConnectionViewModel?.GetSharedBus();
            if (bus != null)
            {
                // Submit request
                var request = bus.Add<SetPipelineInstrumentationMessage>();
                request.featureBitSet = 0x0;
                request.pipelineUID = Pipeline.GUID;
            
                // Track
                _instrumentationState = new();
            }
            
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
            var request = stream.Add<SetPipelineInstrumentationMessage>();
            request.featureBitSet = InstrumentationState.FeatureBitMask;
            request.pipelineUID = Pipeline.GUID;
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
        /// Tracked state
        /// </summary>
        private InstrumentationState _instrumentationState = new();
    }
}