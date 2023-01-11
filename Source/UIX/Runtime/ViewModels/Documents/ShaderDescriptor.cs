using System;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Documents
{
    public class ShaderDescriptor : IDescriptor
    {
        /// <summary>
        /// Sortable identifier
        /// </summary>
        public object? Identifier => Tuple.Create(typeof(ShaderDescriptor), GUID);

        /// <summary>
        /// Workspace collection
        /// </summary>
        public IPropertyViewModel? PropertyCollection { get; set; }

        /// <summary>
        /// Shader GUID
        /// </summary>
        public UInt64 GUID { get; set; }
    }
}