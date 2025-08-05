using System;
using System.Globalization;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using GRS.Features.Debug.UIX.ViewModels;

namespace UIX.Views.Controls;

public class StructuredGrid : Control
{
    ///
    /// This is a big work in progress
    /// Truthfully, I am not sure that any kind of immediate drawing will suffice for our use cases,
    /// it's entirely possible we go with a custom renderer.
    /// 
    
    /// <summary>
    /// Current X-scrolling offset
    /// </summary>
    public float OffsetX { get; set; } = 0;
    
    /// <summary>
    /// Current Y-scrolling offset
    /// </summary>
    public float OffsetY { get; set; } = 0;

    /// <summary>
    /// Assumed width of each cell
    /// </summary>
    public int CellWidth { get; set; } = 25;

    /// <summary>
    /// Invoked on measures
    /// </summary>
    protected override Size MeasureOverride(Size availableSize)
    {
        if (DataContext is not StructuredBreakpointDisplayViewModel viewModel)
        {
            return new Size(0, 0);
        }

        // Just assume the max bounds
        return new Size(
            viewModel.FlatInfo.dataStaticWidth * CellWidth,
            viewModel.FlatInfo.dataStaticHeight * CellWidth
        );
    }

    /// <summary>
    /// Invoked on rendering
    /// </summary>
    public override void Render(DrawingContext context)
    {
        if (DataContext is not StructuredBreakpointDisplayViewModel viewModel || viewModel is { DWords: null })
        {
            return;
        }

        // Actual rendering bounds
        Rect visualBounds = GetVisibleBounds();

        // Cell wise offset
        int cellOffsetX = (int)(OffsetX / CellWidth);
        int cellOffsetY = (int)(OffsetY / CellWidth);

        // Cell wise counts
        int cellCountX = (int)((visualBounds.Width + CellWidth - 1) / CellWidth);
        int cellCountY = (int)((visualBounds.Height + CellWidth - 1) / CellWidth);
        
        // Fill the shared cell data
        var data = new CellData[cellCountX * cellCountY];
        for (int cellRelativeY = 0; cellRelativeY < cellCountY; cellRelativeY++)
        {
            for (int cellRelativeX = 0; cellRelativeX < cellCountX; cellRelativeX++)
            {
                int cellX = cellRelativeX + cellOffsetX;
                int cellY = cellRelativeY + cellOffsetY;
                
                // Start of the streamed data
                int dwordStart = (int)(viewModel.FlatInfo.dataDWordStride * (viewModel.FlatInfo.dataStaticWidth * cellY + cellX));

                data[cellRelativeY * cellCountX + cellRelativeX] = new CellData()
                {
                    Value = viewModel.DWords[dwordStart]
                };
            }
        }
        // Shared typeface
        var typeface = new Typeface("Segoe UI");

        // Create a new bitmap for the 
        var bitmap = new WriteableBitmap(
            new PixelSize((int)visualBounds.Width,  (int)visualBounds.Height),
            new Vector(96, 96),
            PixelFormat.Rgba8888,
            AlphaFormat.Opaque
        );
        
        // Write the background data as one bitmap
        // Far faster then manually blitting each region
        using (var fb = bitmap.Lock())
        {
            unsafe
            {
                uint* ptr = (uint*)fb.Address.ToPointer();
                for (int y = 0; y < bitmap.PixelSize.Height; y++)
                {
                    for (int x = 0; x < bitmap.PixelSize.Width; x++)
                    {
                        int cellX = x / CellWidth;
                        int cellY = y / CellWidth;
                        
                        CellData cell = data[cellY * cellCountX + cellX];
                        *(ptr++) = cell.Value;
                    }
                }
            }
        }
        
        // Blit it!
        context.DrawImage(bitmap, new Rect(new Point(OffsetX, OffsetY), visualBounds.Size));

        // Fill the cell bound geometry
        var geometry = new StreamGeometry();
        using (StreamGeometryContext ctx = geometry.Open())
        {
            for (int cellY = cellOffsetY; cellY < cellOffsetY + cellCountY; cellY++)
            {
                for (int cellX = cellOffsetX; cellX < cellOffsetX + cellCountX; cellX++)
                {
                    float x = cellX * CellWidth;
                    float y = cellY * CellWidth;

                    ctx.BeginFigure(new Point(x, y), false); 
                    ctx.LineTo(new Point(x, y + CellWidth));
                    ctx.LineTo(new Point(x + CellWidth, y + CellWidth));
                    ctx.LineTo(new Point(x + CellWidth, y));
                    ctx.EndFigure(true);
                }
            }
        }

        // Render the cell bounds
        context.DrawGeometry(Brushes.Black, new Pen(Brushes.Gray), geometry);

        // Overlay text rendering
        for (int cellRelativeY = 0; cellRelativeY < cellCountY; cellRelativeY++)
        {
            for (int cellRelativeX = 0; cellRelativeX < cellCountX; cellRelativeX++)
            {
                CellData cell = data[cellRelativeY * cellCountX + cellRelativeX];
                
                float positionX = (cellRelativeX + cellOffsetX) * CellWidth;
                float positionY = (cellRelativeY + cellOffsetY) * CellWidth;

                // Format the cell's contents
                var formatted = new FormattedText(
                    (cell.Value & 0xFF).ToString(),
                    CultureInfo.CurrentCulture,
                    FlowDirection.LeftToRight,
                    typeface,
                    12,
                    Brushes.White
                );

                // Scale the cell formatting by its max bounds
                double scale = Math.Min(
                    1.0f,
                    Math.Min(
                        CellWidth * 0.8 / formatted.Width,
                        CellWidth / formatted.Height
                    )
                ); 

                // Render scaled text
                using var state = context.PushTransform(Matrix.CreateScale(scale, scale) * Matrix.CreateTranslation(positionX, positionY));
                context.DrawText(formatted, new Point(0, 0));
            }
        }
    }

    /// <summary>
    /// Get the assumed visibile bounds for rendering
    /// </summary>
    /// <returns></returns>
    private Rect GetVisibleBounds()
    {
        return (Parent as Control)?.Bounds ?? new Rect();
    }
    
    public struct CellData
    {
        /// <summary>
        /// Dummy value for testing
        /// </summary>
        public uint Value;
    }
}