using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Properties.Config
{
    public class InstrumentationConfigViewModel : BasePropertyViewModel, IInstrumentationProperty
    {
        /// <summary>
        /// Enables safe-guarding on potentially offending instructions
        /// </summary>
        [PropertyField]
        public bool SafeGuard
        {
            get => _safeGuard;
            set
            {
                this.RaiseAndSetIfChanged(ref _safeGuard, value);
                this.EnqueueFirstParentBus();
            }
        }
        
        /// <summary>
        /// Enables detailed instrumentation
        /// </summary>
        [PropertyField]
        public bool Detail
        {
            get => _detail;
            set
            {
                this.RaiseAndSetIfChanged(ref _detail, value);
                this.EnqueueFirstParentBus();
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public InstrumentationConfigViewModel() : base("Instrumentation", PropertyVisibility.Configuration)
        {
            
        }

        /// <summary>
        /// Commit all changes
        /// </summary>
        /// <param name="state"></param>
        public void Commit(InstrumentationState state)
        {
            // Reduce stream size if not needed
            if (!_safeGuard && !_detail)
            {
                return;
            }
            
            // Submit request
            var request = state.SpecializationStream.Add<SetInstrumentationConfigMessage>();
            request.safeGuard = _safeGuard ? 1 : 0;
            request.detail = _detail ? 1 : 0;
        }

        /// <summary>
        /// Internal safe-guard state
        /// </summary>
        private bool _safeGuard = false;

        /// <summary>
        /// Internal detail state
        /// </summary>
        private bool _detail = false;
    }
}