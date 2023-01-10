using ReactiveUI;

namespace Runtime.ViewModels.Shader
{
    public class ShaderFileViewModel : ReactiveObject
    {
        /// <summary>
        /// Contents of this shader
        /// </summary>
        public string Filename
        {
            get => _filename;
            set => this.RaiseAndSetIfChanged(ref _filename, value);
        }

        /// <summary>
        /// Contents of this shader
        /// </summary>
        public string Contents
        {
            get => _contents;
            set => this.RaiseAndSetIfChanged(ref _contents, value);
        }

        /// <summary>
        /// Contents of this shader
        /// </summary>
        public uint UID { get; set; }
        
        /// <summary>
        /// Internal contents
        /// </summary>
        private string _contents = string.Empty;

        /// <summary>
        /// Internal filename
        /// </summary>
        private string _filename = string.Empty;
    }
}