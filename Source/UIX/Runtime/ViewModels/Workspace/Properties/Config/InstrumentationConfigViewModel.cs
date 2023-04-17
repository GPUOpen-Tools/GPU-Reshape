using Message.CLR;
using ReactiveUI;

namespace Studio.ViewModels.Workspace.Properties.Config
{
    public class InstrumentationConfigViewModel : BasePropertyViewModel
    {
        /// <summary>
        /// Enables shader compilation stalling before use
        /// </summary>
        [PropertyField]
        public bool SynchronousRecording
        {
            get => _synchronousRecording;
            set
            {
                this.RaiseAndSetIfChanged(ref _synchronousRecording, value);
                Commit();
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public InstrumentationConfigViewModel() : base("Instrumentation", PropertyVisibility.Configuration)
        {
            
        }

        /// <summary>
        /// Invoked on commits
        /// </summary>
        private void Commit()
        {
            // Get bus
            var bus = ConnectionViewModel?.GetSharedBus();
            if (bus == null)
            {
                return;
            }

            // Submit request
            var request = bus.Add<SetInstrumentationConfigMessage>();
            request.synchronousRecording = _synchronousRecording ? 1 : 0;
        }

        
        /// <summary>
        /// Internal recording state
        /// </summary>
        private bool _synchronousRecording = false;
    }
}