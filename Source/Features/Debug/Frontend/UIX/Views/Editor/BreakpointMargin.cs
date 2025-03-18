using System.Linq;
using Avalonia;
using Avalonia.Input;
using Avalonia.Media;
using AvaloniaEdit.Editing;
using AvaloniaEdit.Rendering;
using GRS.Features.Debug.UIX.ViewModels;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Shader;

namespace UIX.Views.Editor;

public class BreakpointMargin : AbstractMargin
{
    /// <summary>
    /// The textual view model
    /// </summary>
    public ITextualShaderContentViewModel ContentViewModel { get; set; }
    
    /// <summary>
    /// The target collection
    /// </summary>
    public ShaderBreakpointCollectionViewModel CollectionViewModel { get; set; } = new();
    
    /// <summary>
    /// Constructor
    /// </summary>
    static BreakpointMargin()
    {
        FocusableProperty.OverrideDefaultValue(typeof(BreakpointMargin), true);
    }

    /// <summary>
    /// Rendering override
    /// </summary>
    public override void Render(DrawingContext context)
    {
        if (!TextView.VisualLinesValid)
        {
            return;
        }
     
        // TODO[dbg]: Either remove the margin entirely, or actually fill it out right
        context.FillRectangle(new SolidColorBrush(Colors.Black), Bounds);
        context.DrawLine(new Pen(new SolidColorBrush(Colors.White), 0.5), Bounds.TopRight, Bounds.BottomRight);

        // Must have a valid line
        if (TextView.VisualLines.Count <= 0 || TextView.VisualLines.FirstOrDefault() is not { } firstLine)
        {
            return;
        }
        
        // Iterate all lines
        foreach (VisualLine visualLine in TextView.VisualLines)
        {
            int lineNumberBase1 = visualLine.FirstDocumentLine.LineNumber;

            // Has an assigned breakpoint?
            if (CollectionViewModel.Breakpoints.FirstOrDefault(b => b.SourceBinding?.InstructionLine == lineNumberBase1 - 1) is { } breakpoint)
            {
                // Draw breakpoint
                context.FillRectangle(
                    _breakpointBrush,
                    new Rect(
                        Bounds.Size.Width / 4 - 1,
                        visualLine.GetTextLineVisualYPosition(visualLine.TextLines[0], VisualYPosition.LineTop) + (Bounds.Size.Width / 4) - TextView.VerticalOffset,
                        Bounds.Size.Width / 1.5, 
                        firstLine.Height / 1.5
                    ),
                    (float)firstLine.Height
                );

                // Do not render preview
                continue;
            }

            // Breakpoint preview
            if (_previewLine != null && _previewLine.Value == lineNumberBase1)
            {
                context.FillRectangle(
                    _breakpointPreviewBrush,
                    new Rect(
                        Bounds.Size.Width / 4 - 1,
                        visualLine.GetTextLineVisualYPosition(visualLine.TextLines[0], VisualYPosition.LineTop) + Bounds.Size.Width / 4 - TextView.VerticalOffset,
                        Bounds.Size.Width / 1.5,
                        firstLine.Height / 1.5
                    ),
                    (float)firstLine.Height
                );
            }
        }
    }
    
    /// <summary>
    /// Invoked on pointer moves
    /// </summary>
    protected override void OnPointerMoved(PointerEventArgs e)
    {
        // On a valid line? Set preview line
        if (TextView.GetVisualLineFromVisualTop(TextView.ScrollOffset.Y + e.GetPosition(this).Y) is {} visualLine)
        {
            _previewLine = visualLine.FirstDocumentLine.LineNumber;
        }
        else
        {
            _previewLine = null;
        }

        InvalidateVisual();
    }

    /// <summary>
    /// Invoked on releases
    /// </summary>
    protected override void OnPointerReleased(PointerReleasedEventArgs e)
    {
        // Over a valid line?
        if (TextView.GetVisualLineFromVisualTop(TextView.ScrollOffset.Y + e.GetPosition(this).Y) is not { } visualLine)
        {
            return;
        }

        int lineBase0 = visualLine.FirstDocumentLine.LineNumber - 1;

        // If there's a breakpoint, remove it
        if (CollectionViewModel.Breakpoints.FirstOrDefault(x => x.SourceBinding?.InstructionLine == lineBase0) is { } breakpoint)
        {
            CollectionViewModel.Breakpoints.Remove(breakpoint);
        }
        else
        {
            // Find the instruction representing the current line
            AssembledInstructionMapping mapping = ContentViewModel.TransformInstruction(lineBase0);
            
            // None found, add it
            CollectionViewModel.Breakpoints.Add(new BreakpointViewModel
            {
                SourceBinding = new SourceBinding
                {
                    InstructionLine = lineBase0,
                    InstructionCodeOffset = mapping.CodeOffset
                }
            });
        }

        InvalidateVisual();
    }
    
    /// <summary>
    /// Get the control measure
    /// </summary>
    protected override Size MeasureOverride(Size availableSize)
    {
        return TextView != null ? new Size(TextView.DefaultLineHeight, 0) : new Size(0, 0);
    }

    /// <summary>
    /// TODO: Expose styles per plugin
    /// </summary>
    private readonly IBrush _breakpointBrush = Brush.Parse("#DB5C5C");
    private readonly IBrush _breakpointPreviewBrush = Brush.Parse("#653939");

    /// <summary>
    /// Current preview line
    /// </summary>
    private int? _previewLine;
}
