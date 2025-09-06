using System;
using Avalonia.Controls;
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
            .Subscribe(x =>
            {
                Image.Events().PointerMoved.Subscribe(e => OnImagePointerMoved(x, e));
            });
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