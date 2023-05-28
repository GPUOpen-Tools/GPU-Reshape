using ReactiveUI;
using Studio.ViewModels.Workspace.Objects;

namespace Runtime.ViewModels.Shader
{
    public class ShaderNavigationViewModel : ReactiveObject
    {
        /// <summary>
        /// Target shader
        /// </summary>
        public ShaderViewModel? Shader
        {
            get => _shader;
            set => this.RaiseAndSetIfChanged(ref _shader, value);
        }

        /// <summary>
        /// Currently selected file
        /// </summary>
        public ShaderFileViewModel? SelectedFile
        {
            get => _selectedFile;
            set => this.RaiseAndSetIfChanged(ref _selectedFile, value);
        }
        
        /// <summary>
        /// Internal shader
        /// </summary>
        private ShaderViewModel? _shader;
        
        /// <summary>
        /// Internal file
        /// </summary>
        private ShaderFileViewModel? _selectedFile;
    }
}