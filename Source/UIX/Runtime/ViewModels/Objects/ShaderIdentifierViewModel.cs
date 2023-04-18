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
    public class ShaderIdentifierViewModel : ReactiveObject, IInstrumentableObject
    {
        /// <summary>
        /// Underlying model
        /// </summary>
        public ShaderIdentifier Model;

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
        /// Instrumentation handler
        /// </summary>
        public InstrumentationState InstrumentationState
        {
            get
            {
                return Workspace?.PropertyCollection
                    .GetProperty<IShaderCollectionViewModel>()?
                    .GetPropertyWhere<ShaderViewModel>(x => x.Shader.GUID == Model.GUID)?
                    .InstrumentationState ?? new InstrumentationState();   
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
        /// Get the targetable instrumentation property
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel? GetOrCreateInstrumentationProperty()
        {
            // Get collection
            var shaderCollectionViewModel = Workspace?.PropertyCollection.GetProperty<IShaderCollectionViewModel>();
            if (shaderCollectionViewModel == null)
            {
                return null;
            }

            // Find or create property
            var shaderViewModel = shaderCollectionViewModel.GetPropertyWhere<ShaderViewModel>(x => x.Shader.GUID == Model.GUID);
            if (shaderViewModel == null)
            {
                shaderCollectionViewModel.Properties.Add(shaderViewModel = new ShaderViewModel()
                {
                    Parent = shaderCollectionViewModel,
                    ConnectionViewModel = shaderCollectionViewModel.ConnectionViewModel,
                    Shader = Model
                });
            }
            
            // OK
            return shaderViewModel;
        }
    }
}