using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Avalonia;
using Avalonia.Controls;
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
using Runtime.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace UIX.Views.Editor;

public class WatchpointMargin : LineNumberMargin
{
    /// <summary>
    /// Constructor
    /// </summary>
    static WatchpointMargin()
    {
        FocusableProperty.OverrideDefaultValue(typeof(WatchpointMargin), true);
    }

    /// <summary>
    /// Constructor
    /// </summary>
    public WatchpointMargin(Control bottom)
    {
        SetValue(TextBlock.ForegroundProperty, bottom.GetValue(TextBlock.ForegroundProperty));
        SetValue(TextBlock.FontSizeProperty, 14);
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
     
        // Must have a valid line
        if (TextView.VisualLines.Count <= 0 || TextView.VisualLines.FirstOrDefault() is not { } firstLine)
        {
            return;
        }
        
        // Comply with hit tests
        // TODO: I'm not entirely sure why it's missing otherwise when it should hit
        context.DrawRectangle(Brushes.Transparent, null, new Rect(0, 0, Bounds.Width, Bounds.Height));

        // Default watchpoint radius
        double radius = TextView.DefaultLineHeight * 0.65f / 2;
        
        // Iterate all lines
        foreach (VisualLine visualLine in TextView.VisualLines)
        {
            int lineNumberBase1 = visualLine.FirstDocumentLine.LineNumber;

            // Get text positions
            double lineTextTop    = visualLine.GetTextLineVisualYPosition(visualLine.TextLines[0], VisualYPosition.TextTop);
            double lineTextMiddle = visualLine.GetTextLineVisualYPosition(visualLine.TextLines[0], VisualYPosition.TextMiddle);
            
            // Has an assigned watchpoint?
            if (GetWatchpointForLine(lineNumberBase1 - 1) != null)
            {
                // Draw watchpoint
                context.DrawEllipse(
                    _watchpointBrush,
                    new Pen(_watchpointBrush),
                    new Point(
                        Bounds.Size.Width - radius - 1,
                        lineTextMiddle - TextView.VerticalOffset - 3
                    ),
                    radius,
                    radius
                );

                // Do not render preview
                continue;
            }

            // Watchpoint preview
            if (_previewLineBase1 != null && _previewLineBase1.Value == lineNumberBase1)
            {
                context.DrawEllipse(
                    _watchpointPreviewBrush,
                    new Pen(_watchpointBrush),
                    new Point(
                        Bounds.Size.Width - radius - 1,
                        lineTextMiddle - TextView.VerticalOffset - 3
                    ),
                    radius,
                    radius
                );
                
                // Do not render line
                continue;
            }
            
            // Format the line number
            FormattedText text = new FormattedText(
                visualLine.FirstDocumentLine.LineNumber.ToString(CultureInfo.CurrentCulture),
                CultureInfo.CurrentCulture, 
                FlowDirection.LeftToRight, 
                Typeface,
                EmSize,
                GetValue<IBrush>(TextBlock.ForegroundProperty)
            );
            
            // Render it!
            context.DrawText(text, new Point(Bounds.Size.Width - text.Width, lineTextTop - TextView.VerticalOffset));
        }
    }

    /// <summary>
    /// Bind an instruction line
    /// Invalidates when actually bound
    /// </summary>
    private ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel>? BindSourceInstructionLineAssociations(WatchpointViewModelSourceBinding binding)
    {
        // Check cache
        if (!_watchpointSourceAssociations.TryGetValue(binding, out ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel>? association))
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
            
            _watchpointSourceAssociations.Add(binding, association);
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
            VM.LastFocusLineNumberBase0 = VM.LineNumberBase0;
            
            // Assign highlighted watchpoint
            if (GetWatchpointForLine(VM.LineNumberBase0) is { } watchpoint)
            {
                VM.HighlightedWatchpointViewModel = watchpoint;
            }
        }
        else
        {
            _previewLineBase1 = null;
            VM.LineNumberBase0 = 0;
            VM.HighlightedWatchpointViewModel = null;
        }

        InvalidateVisual();
    }

    /// <summary>
    /// Get the watchpoint assigned to a particular line
    /// </summary>
    private WatchpointViewModel? GetWatchpointForLine(int lineBase0)
    {
        // Check textual watchpoints first
        foreach (WatchpointViewModelTextualBinding binding in VM.CollectionViewModel.TextualBindings)
        {
            if (binding.LineBase0 == lineBase0)
            {
                return binding.WatchpointViewModel;
            }
        }

        // Check for source bindings
        foreach (WatchpointViewModelSourceBinding binding in VM.CollectionViewModel.SourceBindings)
        {
            if (IsWatchpointVisible(binding, lineBase0))
            {
                return binding.WatchpointViewModel;
            }
        }

        // No bindings
        return null;
    }

    /// <summary>
    /// Invoked on enters
    /// </summary>
    protected override void OnPointerEntered(PointerEventArgs e)
    {
        base.OnPointerEntered(e);
        Cursor = new Cursor(StandardCursorType.Hand);
    }

    /// <summary>
    /// Invoked on leaves
    /// </summary>
    protected override void OnPointerExited(PointerEventArgs e)
    {
        _previewLineBase1 = null;
        VM.LineNumberBase0 = 0;
        VM.HighlightedWatchpointViewModel = null;
        
        // Re-render the lines
        this.InvalidateVisual();
    }

    /// <summary>
    /// Invoked on presses
    /// </summary>
    protected override void OnPointerPressed(PointerPressedEventArgs e)
    {
        var point = e.GetCurrentPoint(this);

        // Place watchpoint
        if (point.Properties.IsLeftButtonPressed)
        {
             _mode = PlacementMode.New;
            e.Handled = true;
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
            // Handle context manually
            if (e.InitialPressMouseButton == MouseButton.Right)
            {
                if (ContextMenu is IContextMenu contextMenu)
                {
                    contextMenu.PopulateViewModels(this);
                    ContextMenu.Placement = Avalonia.Controls.PlacementMode.Pointer;
                    ContextMenu.Open(this);
                    e.Handled = true;
                    return;
                }
            }
            
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
                HandleNewWatchpoint(lineBase0);
                break;
            default:
                throw new ArgumentOutOfRangeException();
        }

        _mode = PlacementMode.None;
    }

    /// <summary>
    /// Invoked on watchpoint requests
    /// </summary>
    private void HandleNewWatchpoint(int lineBase0)
    {
        // If there's a watchpoint, remove it
        if (GetWatchpointForLine(lineBase0) is { } watchpoint)
        {
            // Deregister against registry
            VM.Content.PropertyCollection?
                .GetService<WatchpointRegistryService>()?
                .Deregister(watchpoint);
            
            // Let the registry handle it, it may be mirrored
            VM.Content.PropertyCollection?
                .GetProperty<WatchpointCollectionRegistryViewModel>()?
                .Remove(watchpoint);
            
            // Remove all bound events
            watchpoint.Disposable.Clear();
        }
        else
        {
            WatchpointUtils.AddWatchpoint(VM.CollectionViewModel, VM.Content, lineBase0, WatchpointCaptureMode.FirstEvent);
        }

        InvalidateVisual();
    }

    /// <summary>
    /// Check if a watchpoint is visible
    /// </summary>
    private bool IsWatchpointVisible(WatchpointViewModelSourceBinding binding, int lineBase0)
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
    private readonly IBrush _watchpointBrush = Brush.Parse("#DB5C5C");
    private readonly IBrush _watchpointPreviewBrush = Brush.Parse("#653939");

    /// <summary>
    /// View model helper
    /// </summary>
    private WatchpointMarginViewModel VM => (WatchpointMarginViewModel)DataContext!;

    /// <summary>
    /// All cached associations
    /// </summary>
    private Dictionary<WatchpointViewModelSourceBinding, ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel>> _watchpointSourceAssociations = new();

    /// <summary>
    /// Current preview line
    /// </summary>
    private int? _previewLineBase1;
}
