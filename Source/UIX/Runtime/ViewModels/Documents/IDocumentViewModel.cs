using Avalonia.Media;

namespace Studio.ViewModels.Documents
{
    public interface IDocumentViewModel
    {
        /// <summary>
        /// Document icon
        /// TODO: Move to generic name, let the actual UI pick up the object
        /// </summary>
        public StreamGeometry? Icon { get; }

        /// <summary>
        /// Document icon color
        /// </summary>
        public Color? IconForeground { get; }
    }
}