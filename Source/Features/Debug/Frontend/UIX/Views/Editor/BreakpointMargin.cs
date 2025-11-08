using System;
using System.Collections.Generic;
using System.Linq;
using Avalonia;
using Avalonia.Input;
using Avalonia.Media;
using Avalonia.Threading;
using AvaloniaEdit.Editing;
using AvaloniaEdit.Rendering;
using DynamicData;
using DynamicData.Binding;
using GRS.Features.Debug.UIX.Models;
using GRS.Features.Debug.UIX.ViewModels;
using GRS.Features.Debug.UIX.ViewModels.Editor;
using GRS.Features.Debug.UIX.ViewModels.Utils;
using GRS.Features.Debug.UIX.Workspace;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.ViewModels.Workspace.Properties;

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
            if (VM.CollectionViewModel.Bindings.FirstOrDefault(b => IsBreakpointVisible(b, lineNumberBase1 - 1)) is { } breakpoint)
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
    /// Bind an instruction line
    /// Invalidates when actually bound
    /// </summary>
    private ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel>? BindSourceInstructionLineAssociations(BreakpointViewModelBinding binding)
    {
        // Check cache
        if (!_breakpointSourceAssociations.TryGetValue(binding, out ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel>? association))
        {
            // Let the content view model handle the instruction -> line of code
            association = VM.Content.TransformInstructionLine(binding.Source.Mapping);
            if (association is null)
            {
                return null;
            }
            
            // Subscribe to all future associations
            association.Associations
                .ToObservableChangeSet()
                .OnItemAdded(pair =>
                {
                    // Invalidate visuals when the location has been mapped
                    if (!pair.Association.Location.HasValue)
                    {
                        pair.Association.WhenAnyValue(x => x.Location).Subscribe(_ =>
                        {
                            // Make sure to invalidate it outside a render loop
                            Dispatcher.UIThread.InvokeAsync(() =>
                            {
                                InvalidateVisual();
                            });
                        });
                    }
                })
                .Subscribe()
                .Dispose();
            
            _breakpointSourceAssociations.Add(binding, association);
        }
        
        return association;
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
            if (VM.CollectionViewModel.Bindings.FirstOrDefault(x => IsBreakpointVisible(x, VM.LineNumberBase0)) is { } breakpoint)
            {
                VM.HighlightedBreakpointViewModel = breakpoint.BreakpointViewModel;
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
        if (VM.CollectionViewModel.Bindings.FirstOrDefault(x => IsBreakpointVisible(x, lineBase0)) is { } breakpoint)
        {
            // Deregister against registry
            VM.Content.PropertyCollection?
                .GetService<BreakpointRegistryService>()?
                .Deregister(breakpoint.BreakpointViewModel);
            
            // Let the registry handle it, it may be mirrored
            VM.Content.PropertyCollection?
                .GetProperty<BreakpointCollectionRegistryViewModel>()?
                .Remove(breakpoint.BreakpointViewModel);
        }
        else
        {
            BreakpointUtils.AddBreakpoint(VM.CollectionViewModel, VM.Content, lineBase0, BreakpointCaptureMode.FirstEvent);
        }

        InvalidateVisual();
    }

    /// <summary>
    /// Check if a breakpoint is visible
    /// </summary>
    private bool IsBreakpointVisible(BreakpointViewModelBinding binding, int lineBase0)
    {
        if (BindSourceInstructionLineAssociations(binding) is not { } associationViewModel)
        {
            return false;
        }
        
        // Check all associations
        foreach (ShaderMultiAssociationPair<ShaderInstructionSourceAssociationViewModel> pair in associationViewModel.Associations)
        {
            // May not be bound yet
            if (pair.Association.Location is not { } location)
            {
                continue;
            }
            
            // Check the line
            if (lineBase0 == location.Line && VM.Content.IsLocationVisible(location))
            {
                return true;
            }
        }

        // Irrelevant
        return false;
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
    /// All cached associations
    /// </summary>
    private Dictionary<BreakpointViewModelBinding, ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel>> _breakpointSourceAssociations = new();

    /// <summary>
    /// Current preview line
    /// </summary>
    private int? _previewLineBase1;
}
