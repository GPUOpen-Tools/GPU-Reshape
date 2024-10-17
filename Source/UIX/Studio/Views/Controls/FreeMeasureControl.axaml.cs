using System;
using Avalonia;
using Avalonia.Controls;
using ReactiveUI;

namespace Studio.Views.Controls;

public partial class FreeMeasureControl : UserControl
{
    public FreeMeasureControl()
    {
        InitializeComponent();

        // We're technically rendering beyond the measured bounds, so don't clip
        ClipToBounds = false;
    }

    protected override void OnAttachedToVisualTree(VisualTreeAttachmentEventArgs e)
    {
        base.OnAttachedToVisualTree(e);

        // We're dependent on the parents effective bounds
        // Every arrange is one invalidation off, so, re-trigger the arranges
        e.Parent.WhenAnyValue(x => x.Bounds).Subscribe(_ =>  InvalidateArrange());
    }
    
    protected override Size MeasureOverride(Size availableSize)
    {
        // Let the children measure by the potentially infinite bounds
        if (Parent is Control parentVisual)
        {
            availableSize = new Size(availableSize.Width, Math.Max(MinHeight, parentVisual.Bounds.Height) - Padding.Bottom);
        }

        // Measure children, but ignore the output
        base.MeasureOverride(availableSize);
        
        // Report the min dimensions to the parent measure
        return new Size(availableSize.Width, MinHeight);
    }

    protected override Size ArrangeOverride(Size finalSize)
    {
        // Arrange by the effective dimensions, not measured
        if (Parent is Control parentVisual)
        {
            finalSize = new Size(finalSize.Width, Math.Max(MinHeight, parentVisual.Bounds.Height) - Padding.Bottom);
        }
        
        // Arrange as usual
        return base.ArrangeOverride(finalSize);
    }
}