using System;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Input;
using Avalonia.Media;
using GRS.Features.Debug.UIX.ViewModels;
using ReactiveUI;
using Studio.Extensions;
using Point = Avalonia.Point;

namespace UIX.Views.Display;

public partial class ImageBreakpointDisplayView : UserControl, IViewFor
{
    public object? ViewModel
    {
        get => DataContext;
        set => DataContext = value;
    }

    public ImageBreakpointDisplayView()
    {
        InitializeComponent();

        // Bind view model
        this.WhenAnyValue(x => x.DataContext)
            .CastNullable<ImageBreakpointDisplayViewModel>()
            .WhereNotNull()
            .Subscribe(vm =>
            {
                // Bind lock changes
                vm.WhenAnyValue(x => x.LockToContent).Subscribe(x => OnLockChanged(vm));
                
                // Reapply lock when the image has changed
                vm.WhenAnyValue(x => x.Image).Subscribe(x => OnLockChanged(vm));

                // Bind pointer move
                Image.Events().PointerMoved.Subscribe(e => OnImagePointerMoved(vm, e));
            });
    }
    
    /// <summary>
    /// Invoked on lock changes
    /// </summary>
    private void OnLockChanged(ImageBreakpointDisplayViewModel viewModel)
    {
        if (viewModel.LockToContent)
        {
            Image.Width = ScrollArea.Bounds.Width;
            Image.Height = ScrollArea.Bounds.Height;

            ScrollArea.HorizontalScrollBarVisibility = ScrollBarVisibility.Hidden;
            ScrollArea.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;
        }
        else
        {
            Image.Width = Image.Source?.Size.Width ?? 1.0;
            Image.Height = Image.Source?.Size.Height ?? 1.0;

            ScrollArea.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
            ScrollArea.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
        }
    }

    /// <summary>
    /// Invoked on pointer events
    /// </summary>
    private void OnImagePointerMoved(ImageBreakpointDisplayViewModel viewModel, PointerEventArgs e)
    {
        Point point = e.GetPosition(Image);

        // Get factors
        double factorX = point.X / Image.Bounds.Width;
        double factorY = point.Y / Image.Bounds.Height;

        // Get source coordinates
        uint imageX = (uint)((Image.Source?.Size.Width ?? 1) * factorX);
        uint imageY = (uint)((Image.Source?.Size.Height ?? 1) * factorY);

        // Let the inspector render the value
        PixelInspectionRender pixel = viewModel.Inspector?.Inspect(imageX, imageY) ?? new PixelInspectionRender()
        {
            Color = Colors.Transparent,
            NativeFormatRender = "Invalid"
        };

        // Update view model
        viewModel.PixelDecoration = $"Pixel X:{imageX} Y:{imageY} - {pixel.NativeFormatRender}";
        viewModel.PixelColor = new SolidColorBrush(pixel.Color);
    }
}