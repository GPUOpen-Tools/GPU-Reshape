using System;
using System.Linq;
using Avalonia;
using Avalonia.Input;
using Avalonia.Media;
using AvaloniaEdit.Editing;
using AvaloniaEdit.Rendering;
using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.ViewModels.Editor;
using GRS.Features.Debug.UIX.ViewModels.Utils;

namespace UIX.Views.Editor;

public class BreakpointMargin : AbstractMargin
{
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
            if (VM.CollectionViewModel.Breakpoints.FirstOrDefault(b => b.SourceBinding?.InstructionLine == lineNumberBase1 - 1) is { } breakpoint)
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
            if (_previewLineBase1 != null && _previewLineBase1.Value == lineNumberBase1)
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
            _previewLineBase1  = visualLine.FirstDocumentLine.LineNumber;
            VM.LineNumberBase0 = (int)(_previewLineBase1 - 1);
            
            // Assign highlighted breakpoint
            if (VM.CollectionViewModel.Breakpoints.FirstOrDefault(x => x.SourceBinding?.InstructionLine == VM.LineNumberBase0) is { } breakpoint)
            {
                VM.HighlightedBreakpointViewModel = breakpoint;
            }
        }
        else
        {
            _previewLineBase1 = null;
            VM.LineNumberBase0 = 0;
            VM.HighlightedBreakpointViewModel = null;
        }

        InvalidateVisual();
    }

    /// <summary>
    /// Invoked on presses
    /// </summary>
    protected override void OnPointerPressed(PointerPressedEventArgs e)
    {
        var point = e.GetCurrentPoint(this);

        // Place breakpoint
        if (point.Properties.IsLeftButtonPressed)
        {
             _mode = PlacementMode.New;
            e.Handled = true;
        }

        // Otherwise pass down
        if (!e.Handled)
        {
            base.OnPointerPressed(e);
        }
    }

    /// <summary>
    /// Invoked on releases
    /// </summary>
    protected override void OnPointerReleased(PointerReleasedEventArgs e)
    {
        // Pass down if not our event
        if (_mode == PlacementMode.None)
        {
            base.OnPointerReleased(e);
            return;
        }
        
        // Over a valid line?
        if (TextView.GetVisualLineFromVisualTop(TextView.ScrollOffset.Y + e.GetPosition(this).Y) is not { } visualLine)
        {
            return;
        }

        int lineBase0 = visualLine.FirstDocumentLine.LineNumber - 1;

        // Handle mode
        switch (_mode)
        {
            case PlacementMode.New:
                HandleNewBreakpoint(lineBase0);
                break;
            default:
                throw new ArgumentOutOfRangeException();
        }

        _mode = PlacementMode.None;
    }

    /// <summary>
    /// Invoked on breakpoint requests
    /// </summary>
    private void HandleNewBreakpoint(int lineBase0)
    {
        // If there's a breakpoint, remove it
        if (VM.CollectionViewModel.Breakpoints.FirstOrDefault(x => x.SourceBinding?.InstructionLine == lineBase0) is { } breakpoint)
        {
            VM.CollectionViewModel.Breakpoints.Remove(breakpoint);
        }
        else
        {
            BreakpointUtils.AddBreakpoint(VM.CollectionViewModel, VM.ContentViewModel, lineBase0, BreakpointCaptureMode.FirstEvent);
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
    /// Mode types
    /// </summary>
    private enum PlacementMode
    {
        None,
        New
    }
    
    /// <summary>
    /// Current placement mode
    /// </summary>
    private PlacementMode _mode = PlacementMode.None;

    /// <summary>
    /// TODO: Expose styles per plugin
    /// </summary>
    private readonly IBrush _breakpointBrush = Brush.Parse("#DB5C5C");
    private readonly IBrush _breakpointPreviewBrush = Brush.Parse("#653939");

    /// <summary>
    /// View model helper
    /// </summary>
    private BreakpointMarginViewModel VM => (BreakpointMarginViewModel)DataContext!;

    /// <summary>
    /// Current preview line
    /// </summary>
    private int? _previewLineBase1;
}
