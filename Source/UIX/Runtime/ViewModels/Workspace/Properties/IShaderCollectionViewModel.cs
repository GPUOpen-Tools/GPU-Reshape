
using System;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Properties
{
    public interface IShaderCollectionViewModel : IPropertyViewModel
    {
        /// <summary>
        /// Add a new shader to this collection
        /// </summary>
        /// <param name="shaderViewModel"></param>
        public void AddShader(ShaderViewModel shaderViewModel);

        /// <summary>
        /// Get a shader from this collection
        /// </summary>
        /// <param name="GUID"></param>
        /// <returns>null if not found</returns>
        public ShaderViewModel? GetShader(UInt64 GUID);

        /// <summary>
        /// Get a shader from this collection, add if not found
        /// </summary>
        /// <param name="GUID"></param>
        /// <returns></returns>
        public ShaderViewModel GetOrAddShader(UInt64 GUID);
    }
}