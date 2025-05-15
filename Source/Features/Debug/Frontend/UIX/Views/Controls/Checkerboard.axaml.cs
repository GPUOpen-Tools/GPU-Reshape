using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;

namespace UIX.Views.Controls;

public class Checkerboard : Control
{
    /// <summary>
    /// Color on light tiles
    /// </summary>
    public IBrush LightColor { get; set; } = Brushes.LightGray;
    
    /// <summary>
    /// Color on dark tiles
    /// </summary>
    public IBrush DarkColor { get; set; } = Brushes.Gray;
 
    /// <summary>
    /// The width of a given tile
    /// </summary>
    public int TileWidth { get; set; } = 8;
    
    /// <summary>
    /// Invoked on draws
    /// </summary>
    public override void Render(DrawingContext context)
    {
        var columnCount = (int)Math.Ceiling(Bounds.Width / TileWidth);
        var rowCount    = (int)Math.Ceiling(Bounds.Height / TileWidth);

        // TODO[dbg]: Validate draw persistence
        for (int y = 0; y < rowCount; y++)
        {
            for (int x = 0; x < columnCount; x++)
            {
                context.FillRectangle(
                    (x + y) % 2 != 0 ? LightColor : DarkColor, 
                    new Rect(x * TileWidth, y * TileWidth, TileWidth, TileWidth)
                );
            }
        }
    }
}