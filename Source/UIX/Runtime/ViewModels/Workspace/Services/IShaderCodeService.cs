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
        public void EnqueueShaderContents(Objects.ShaderViewModel shaderViewModel);
        
        /// <summary>
        /// Enqueue a shader for source code pooling
        /// </summary>
        /// <param name="shaderViewModel"></param>
        public void EnqueueShaderIL(Objects.ShaderViewModel shaderViewModel);
        
        /// <summary>
        /// Enqueue a shader for source code pooling
        /// </summary>
        /// <param name="shaderViewModel"></param>
        public void EnqueueShaderBlockGraph(Objects.ShaderViewModel shaderViewModel);
    }
}