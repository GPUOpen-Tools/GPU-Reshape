using Avalonia.Media;

namespace Studio.ViewModels.Documents
{
    public interface IDocumentViewModel
    {
        /// <summary>
        /// Descriptor setter, constructs the top document from given descriptor
        /// </summary>
        public IDescriptor? Descriptor { set; }
        
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