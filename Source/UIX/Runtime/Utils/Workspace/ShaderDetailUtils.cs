using DynamicData;
using Runtime.Models.Objects;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Config;

namespace Runtime.Utils.Workspace
{
    public static class ShaderDetailUtils
    {
        /// <summary>
        /// Begin detailed collection of shader data
        /// </summary>
        public static void BeginDetailedCollection(ShaderViewModel _object, IPropertyViewModel propertyCollection)
        {
            // Get collection
            var shaderCollectionViewModel = propertyCollection.GetProperty<IShaderCollectionViewModel>();
            if (shaderCollectionViewModel == null)
            {
                return;
            }

            // Find or create property
            var shaderViewModel = shaderCollectionViewModel.GetPropertyWhere<Studio.ViewModels.Workspace.Properties.Instrumentation.ShaderViewModel>(x => x.Shader.GUID == _object.GUID);
            if (shaderViewModel == null)
            {
                shaderCollectionViewModel.Properties.Add(shaderViewModel = new Studio.ViewModels.Workspace.Properties.Instrumentation.ShaderViewModel()
                {
                    Parent = shaderCollectionViewModel,
                    ConnectionViewModel = shaderCollectionViewModel.ConnectionViewModel,
                    Shader = new ShaderIdentifier()
                    {
                        GUID = _object.GUID,
                        Descriptor = $"Shader {_object.GUID} - {System.IO.Path.GetFileName(_object.Filename)}"
                    }
                });
            }

            // Enable detailed instrumentation on the shader
            if (shaderViewModel.GetProperty<InstrumentationConfigViewModel>() is { } instrumentationConfigViewModel && !instrumentationConfigViewModel.Detail)
            {
                instrumentationConfigViewModel.Detail = true;
            }
        }
    }
}