using System;

namespace Studio.Models.Workspace.Objects
{
    [Flags]
    public enum AsyncShaderStatus
    {
        None = 0,
        
        /// <summary>
        /// Given shader could not be found on the target device
        /// </summary>
        NotFound = 1 << 0,
        
        /// <summary>
        /// Shader has no debugging symbols, IL is available
        /// </summary>
        NoDebugSymbols = 1 << 1,
    }
}