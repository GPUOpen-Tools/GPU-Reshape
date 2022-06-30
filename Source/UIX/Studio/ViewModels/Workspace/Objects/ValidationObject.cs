using ReactiveUI;
using Studio.Models.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Objects
{
    public class ValidationObject : ReactiveObject
    {
        /// <summary>
        /// Number of messages
        /// </summary>
        public uint Count
        {
            get => _count;
            set => this.RaiseAndSetIfChanged(ref _count, value);
        }

        /// <summary>
        /// Shader wise location of this object
        /// </summary>
        public ShaderLocation Location { get; set; }

        /// <summary>
        /// General contents
        /// </summary>
        public string Content
        {
            get => _content;
            set => this.RaiseAndSetIfChanged(ref _content, value);
        }

        /// <summary>
        /// Internal count
        /// </summary>
        private uint _count;

        /// <summary>
        /// Internal content
        /// </summary>
        private string _content = string.Empty;
    }
}