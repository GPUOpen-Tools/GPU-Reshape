using Message.CLR;
using ReactiveUI;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Properties.Config
{
    public class ApplicationInstrumentationConfigViewModel : BasePropertyViewModel, IBusObject
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
                this.EnqueueBus();
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public ApplicationInstrumentationConfigViewModel() : base("Instrumentation", PropertyVisibility.Configuration)
        {
            
        }

        /// <summary>
        /// Commit all state
        /// </summary>
        /// <param name="stream"></param>
        public void Commit(OrderedMessageView<ReadWriteMessageStream> stream)
        {
            // Submit request
            var request = stream.Add<SetApplicationInstrumentationConfigMessage>();
            request.synchronousRecording = _synchronousRecording ? 1 : 0;
        }

        /// <summary>
        /// Internal recording state
        /// </summary>
        private bool _synchronousRecording = false;
    }
}