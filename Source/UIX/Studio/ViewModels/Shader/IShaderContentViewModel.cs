using System.Windows.Input;
using Avalonia.Media;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Shader
{
    public interface IShaderContentViewModel
    {
        /// <summary>
        /// Given creation descriptor
        /// </summary>
        public ShaderDescriptor? Descriptor { set; }
        
        /// <summary>
        /// Content icon
        /// </summary>
        public StreamGeometry? Icon { get; set; }
        
        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public IPropertyViewModel? PropertyCollection { get; set; }

        /// <summary>
        /// Selection command
        /// </summary>
        public ICommand? OnSelected { get; }
        
        /// <summary>
        /// Is this model active?
        /// </summary>
        public bool IsActive { get; set; }

        /// <summary>
        /// Underlying object
        /// </summary>
        public Workspace.Objects.ShaderViewModel? Object { get; set; }
    }
}
