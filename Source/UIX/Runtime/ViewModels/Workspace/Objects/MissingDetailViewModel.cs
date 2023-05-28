using DynamicData;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Config;

namespace Studio.ViewModels.Workspace.Objects
{
    public class MissingDetailViewModel : ReactiveObject, IValidationDetailViewModel
    {
        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public IPropertyViewModel? PropertyCollection
        {
            get => _propertyCollection;
            set
            {
                this.RaiseAndSetIfChanged(ref _propertyCollection, value);
                OnChanged();
            }
        }

        /// <summary>
        /// Underlying object
        /// </summary>
        public ShaderViewModel? Object
        {
            get => _object;
            set
            {
                this.RaiseAndSetIfChanged(ref _object, value);
                OnChanged();
            }
        }

        /// <summary>
        /// Invoked on changes
        /// </summary>
        private void OnChanged()
        {
            if (Object == null || PropertyCollection == null)
            {
                return;
            }
            
            // Get collection
            var shaderCollectionViewModel = PropertyCollection.GetProperty<IShaderCollectionViewModel>();
            if (shaderCollectionViewModel == null)
            {
                return;
            }

            // Find or create property
            var shaderViewModel = shaderCollectionViewModel.GetPropertyWhere<Properties.Instrumentation.ShaderViewModel>(x => x.Shader.GUID == Object.GUID);
            if (shaderViewModel == null)
            {
                shaderCollectionViewModel.Properties.Add(shaderViewModel = new Properties.Instrumentation.ShaderViewModel()
                {
                    Parent = shaderCollectionViewModel,
                    ConnectionViewModel = shaderCollectionViewModel.ConnectionViewModel,
                    Shader = new ShaderIdentifier()
                    {
                        GUID = Object.GUID,
                        Descriptor = $"Shader {Object.GUID} - {System.IO.Path.GetFileName(Object.Filename)}"
                    }
                });
            }

            // Enable detailed instrumentation on the shader
            if (shaderViewModel.GetProperty<InstrumentationConfigViewModel>() is { } instrumentationConfigViewModel)
            {
                instrumentationConfigViewModel.Detail = true;
            }
        }
        
        /// <summary>
        /// Internal object
        /// </summary>
        private ShaderViewModel? _object;

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IPropertyViewModel? _propertyCollection;
    }
}