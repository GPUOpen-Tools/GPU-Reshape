using System;
using System.Collections.Generic;
using Message.CLR;

namespace Studio.ViewModels.Workspace.Listeners
{
    public interface IShaderCodeService : IPropertyService
    {
        /// <summary>
        /// Enqueue a shader for source code pooling
        /// </summary>
        /// <param name="shaderViewModel"></param>
        public void EnqueueShader(Objects.ShaderViewModel shaderViewModel);
    }
}