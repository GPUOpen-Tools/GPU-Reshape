using Avalonia.Media;

namespace Runtime.Models.Query
{
    public struct QueryAttributeDecorator
    {
        /// <summary>
        /// Source attribute
        /// </summary>
        public QueryAttribute Attribute { get; set; }
        
        /// <summary>
        /// Decorative color
        /// </summary>
        public Color Color { get; set; }
    }
}