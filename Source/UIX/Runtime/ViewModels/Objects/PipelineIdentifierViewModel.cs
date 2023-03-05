using System;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

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
        /// <param name="filter"></param>
        public void SetInstrumentation(InstrumentationState state)
        {
            // Get collection
            var pipelineCollectionViewModel = Workspace?.PropertyCollection.GetProperty<IPipelineCollectionViewModel>();
            if (pipelineCollectionViewModel == null)
            {
                return;
            }

            // Find or create property
            var pipelineViewModel = pipelineCollectionViewModel.GetPropertyWhere<PipelineViewModel>(x => x.Pipeline.GUID == Model.GUID);
            if (pipelineViewModel == null)
            {
                pipelineCollectionViewModel.Properties.Add(pipelineViewModel = new PipelineViewModel()
                {
                    Parent = pipelineCollectionViewModel,
                    ConnectionViewModel = pipelineCollectionViewModel.ConnectionViewModel,
                    Pipeline = Model
                });
            }
            
            // Pass down
            pipelineViewModel.SetInstrumentation(state);
        }
    }
}