using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Shader
{
    public interface ITextualShaderContentViewModel : IShaderContentViewModel
    {
        /// <summary>
        /// Is the overlay visible?
        /// </summary>
        public bool IsOverlayVisible();
        
        /// <summary>
        /// Is a validation object visible?
        /// </summary>
        public bool IsObjectVisible(ValidationObject validationObject);

        /// <summary>
        /// Transform a shader location line
        /// </summary>
        public int TransformLine(ShaderLocation shaderLocation);
    }
}