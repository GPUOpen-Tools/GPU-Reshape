using System;
using System.Linq;
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
    public class GlobalViewModel : BasePropertyViewModel, IInstrumentableObject, IClosableObject, IBusObject
    {
        /// <summary>
        /// Instrumentation handler
        /// </summary>
        public InstrumentationState InstrumentationState
        {
            get => _instrumentationState;
            set => this.CheckRaiseAndSetIfChanged(ref _instrumentationState, value);
        }

        /// <summary>
        /// Invoked on closes
        /// </summary>
        public ICommand? CloseCommand { get; set; }

        /// <summary>
        /// Constructor
        /// </summary>
        public GlobalViewModel() : base("Global", PropertyVisibility.WorkspaceTool)
        {
            CloseCommand = ReactiveCommand.Create(OnClose);

            // Connect properties
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
        /// Get the owning workspace
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel? GetWorkspace()
        {
            return Parent?.GetRoot();
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
                var request = bus.Add<SetGlobalInstrumentationMessage>();
                request.featureBitSet = 0x0;
            
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
            var request = stream.Add<SetGlobalInstrumentationMessage>();
            request.featureBitSet = _instrumentationState.FeatureBitMask;
        }

        /// <summary>
        /// Tracked state
        /// </summary>
        private InstrumentationState _instrumentationState = new();
    }
}