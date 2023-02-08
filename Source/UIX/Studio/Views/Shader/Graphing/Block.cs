using Avalonia.Media;

namespace Studio.Views.Shader.Graphing
{
    public class Block
    {
        /// <summary>
        /// Name of this block
        /// </summary>
        public string Name { get; set; } = string.Empty;
        
        /// <summary>
        /// Color of this block
        /// </summary>
        public IBrush Color { get; set; }
    }
}