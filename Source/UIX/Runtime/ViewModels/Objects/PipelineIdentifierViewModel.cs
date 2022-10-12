using System;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.ViewModels.Instrumentation;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Runtime.ViewModels.Objects
{
    public class PipelineIdentifierViewModel : ReactiveObject, IInstrumentableObject
    {
        /// <summary>
        /// Underlying model
        /// </summary>
        public PipelineIdentifier Model;

        /// <summary>
        /// Owning workspace
        /// </summary>
        public IWorkspaceViewModel? Workspace;
        
        /// <summary>
        /// Non reactive, identifiers are pooled in a pseudo-virtualized manner
        /// </summary>
        public bool HasBeenPooled { get; set; }

        /// <summary>
        /// Readable descriptor
        /// </summary>
        public string Descriptor
        {
            get => Model.Descriptor;
            set
            {
                if (value != Model.Descriptor)
                {
                    Model.Descriptor = value;
                    this.RaisePropertyChanged(nameof(Descriptor));
                }
            }
        }

        /// <summary>
        /// Global UID
        /// </summary>
        public UInt64 GUID
        {
            get => Model.GUID;
            set
            {
                if (value != Model.GUID)
                {
                    Model.GUID = value;
                    this.RaisePropertyChanged(nameof(GUID));
                }
            }
        }

        /// <summary>
        /// Get the workspace
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel? GetWorkspace()
        {
            return Workspace?.PropertyCollection;
        }

        /// <summary>
        /// Set the instrumentation
        /// </summary>
        /// <param name="state"></param>
        public void SetInstrumentation(InstrumentationState state)
        {
            // Get bus
            var bus = Workspace?.Connection?.GetSharedBus();
            if (bus == null)
                return;

            // Submit request
            var request = bus.Add<SetPipelineInstrumentationMessage>();
            request.pipelineUID = GUID;
            request.featureBitSet = state.FeatureBitMask;
        }
    }
}