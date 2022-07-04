using ReactiveUI;
using Studio.Models.Workspace.Listeners;
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
        /// Shader wise segment of this object
        /// </summary>
        public ShaderSourceSegment? Segment
        {
            get => _segment;
            set
            {
                if (_segment != value)
                {
                    this.RaiseAndSetIfChanged(ref _segment, value);
                    this.RaisePropertyChanged(nameof(Extract));
                }
            }
        }

        /// <summary>
        /// General contents
        /// </summary>
        public string Content
        {
            get => _content;
            set => this.RaiseAndSetIfChanged(ref _content, value);
        }

        /// <summary>
        /// General contents
        /// </summary>
        public string Extract => Segment?.Extract ?? string.Empty;

        /// <summary>
        /// Increment the count without a reactive raise
        /// </summary>
        /// <param name="silentCount"></param>
        public void IncrementCountNoRaise(uint silentCount)
        {
            _count += silentCount;
        }

        /// <summary>
        /// Internal count
        /// </summary>
        private uint _count;

        /// <summary>
        /// Internal content
        /// </summary>
        private string _content = string.Empty;

        private ShaderSourceSegment? _segment;

        /// <summary>
        /// Internal extract
        /// </summary>
        private string _extract = string.Empty;
    }
}