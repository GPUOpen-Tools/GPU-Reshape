using Avalonia.Media;

namespace Studio.Views.Shader.Graphing
{
    public class Edge : AvaloniaGraphControl.Edge
    {
        public Edge(Block tail, Block head) : base(tail, head)
        {
            // ...
        }

        /// <summary>
        /// Color of this edge
        /// </summary>
        public Color Color { get; set; }
    }
}