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
    public class GlobalViewModel : BasePropertyViewModel, IInstrumentableObject, IClosableObject
    {
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
                var request = bus.Add<SetGlobalInstrumentationMessage>();
                request.featureBitSet = value.FeatureBitMask;
            }
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
            var request = bus.Add<SetGlobalInstrumentationMessage>();
            request.featureBitSet = 0x0;
            
            // Track
            _instrumentationState = new();
            
            // Remove from parent
            this.DetachFromParent();
        }

        /// <summary>
        /// Tracked state
        /// </summary>
        private InstrumentationState _instrumentationState = new();
    }
}