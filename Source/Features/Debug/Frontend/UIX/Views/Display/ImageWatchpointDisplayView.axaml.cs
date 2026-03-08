using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.Input;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using GRS.Features.Debug.UIX.ViewModels;
using ReactiveUI;
using Studio.Extensions;
using Point = Avalonia.Point;

namespace UIX.Views.Display;

public partial class ImageWatchpointDisplayView : UserControl, IViewFor
{
    public object? ViewModel
    {
        get => DataContext;
        set => DataContext = value;
    }

    public ImageWatchpointDisplayView()
    {
        InitializeComponent();

        // Bind view model
        this.WhenAnyValue(x => x.DataContext)
            .CastNullable<ImageWatchpointDisplayViewModel>()
            .WhereNotNull()
            .Subscribe(vm =>
            {
                // Bind lock changes
                vm.WhenAnyValue(x => x.LockToContent).Subscribe(x => OnLockChanged(vm));
                
                // Reapply lock when the image has changed
                vm.WhenAnyValue(x => x.Image).Subscribe(x => OnContentChanged(vm));

                // Bind zoom events
                Image.Events().PointerMoved.Subscribe(e => OnZoomPointerMoved(vm, e));
                Image.Events().PointerPressed.Subscribe(e => OnZoomPointerPressed(vm, e));
                Image.Events().PointerReleased.Subscribe(e => OnZoomPointerReleased(vm, e));
                Image.Events().PointerWheelChanged.Subscribe(e => OnZoomPointerWheel(vm, e));
                
                // Bind layout changes
                this.Events().LayoutUpdated.Subscribe(e => OnLayoutChanged(vm));
            });
    }

    /// <summary>
    /// Invoked on layout changes
    /// </summary>
    private void OnLayoutChanged(ImageWatchpointDisplayViewModel vm)
    {
        // Locked content will always reapply
        if (!_pendingLockUpdate && !vm.LockToContent)
        {
            return;
        }
        
        if (vm.LockToContent)
        {
            Image.Margin = new Thickness(0, 0, 0, 0);
            
            // Always scale to the bounds
            Image.Width = ScrollArea.Bounds.Width;
            Image.Height = ScrollArea.Bounds.Height;

            // No scrolling
            ScrollArea.HorizontalScrollBarVisibility = ScrollBarVisibility.Hidden;
            ScrollArea.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;

            _zoomFactor = 1.0f;
        }
        else
        {
            // Reset the zoom factor to "match"
            double factorWidth = ScrollArea.Bounds.Width / Image.Source?.Size.Width ?? 1.0;
            double factorHeight = ScrollArea.Bounds.Height / Image.Source?.Size.Height ?? 1.0;
            _zoomFactor = Math.Min(factorWidth, factorHeight);
            
            // Set new dimensions
            Image.Width = Image.Source?.Size.Width * _zoomFactor ?? 1.0;
            Image.Height = Image.Source?.Size.Height * _zoomFactor ?? 1.0;

            // Default scrolling
            ScrollArea.HorizontalScrollBarVisibility = ScrollBarVisibility.Auto;
            ScrollArea.VerticalScrollBarVisibility = ScrollBarVisibility.Auto;
        }

        _pendingLockUpdate = false;
    }

    /// <summary>
    /// Invoked on lock changes
    /// </summary>
    private void OnLockChanged(ImageWatchpointDisplayViewModel viewModel)
    {
        _pendingLockUpdate = true;
    }

    /// <summary>
    /// Update the image changes
    /// </summary>
    private void OnContentChanged(ImageWatchpointDisplayViewModel vm)
    {
        // Get the source dimensions
        int sourceWidth = (int)(vm.Image?.Size.Width ?? 1);
        int sourceHeight = (int)(vm.Image?.Size.Height ?? 1);
        
        // If the dimensions have changed, reapply the lock
        if (_lastSourceWidth != sourceWidth || _lastSourceHeight != sourceHeight)
        {
            OnLockChanged(vm);
            
            _lastSourceWidth = sourceWidth;
            _lastSourceHeight = sourceHeight;
        }
    }

    /// <summary>
    /// Invoked on scrolling
    /// </summary>
    private void OnZoomPointerWheel(ImageWatchpointDisplayViewModel vm, PointerWheelEventArgs e)
    {
        if (vm.LockToContent)
        {
            vm.LockToContent = false;
            OnLayoutChanged(vm);
        }
        
        Point point = e.GetPosition(ScrollArea);

        // Get the source point
        Point sourcePoint = new(
            (ScrollArea.Offset.X + point.X) / _zoomFactor,
            (ScrollArea.Offset.Y + point.Y) / _zoomFactor
        );

        // Get the source dimensions
        double sourceWidth = vm.Image?.Size.Width ?? 1;
        double sourceHeight = vm.Image?.Size.Height ?? 1;

        // Apply zooming in log scale
        _zoomFactor = Math.Exp(Math.Log(_zoomFactor) + Math.Log(1.2f) * e.Delta.Y);
        _zoomFactor = Math.Clamp(_zoomFactor, 0.01, Math.Max(sourceWidth, sourceHeight));
        
        // Update the image size, pseudo zooming
        Image.Width = sourceWidth * _zoomFactor;
        Image.Height = sourceHeight * _zoomFactor;

        // Figure out the new point after scaling
        Point destPoint = new(
            sourcePoint.X * _zoomFactor - point.X,
            sourcePoint.Y * _zoomFactor - point.Y
        );
        
        // Assign new scrolling offset
        ScrollArea.Offset = new Vector(
            Math.Clamp(destPoint.X, 0.0, Math.Max(0, Image.Width - ScrollArea.Viewport.Width)),
            Math.Clamp(destPoint.Y, 0.0, Math.Max(0, Image.Height - ScrollArea.Viewport.Height))
        );

        // Update the display mode
        UpdateInterpolationMode(vm);

        e.Handled = true;
    }

    /// <summary>
    /// Update the interpolation mode
    /// </summary>
    private void UpdateInterpolationMode(ImageWatchpointDisplayViewModel vm)
    {
        // Get the source dimensions
        double sourceWidth = vm.Image?.Size.Width ?? 1;
        double sourceHeight = vm.Image?.Size.Height ?? 1;
        
        // Figure out how many pixels are actually visible
        double visiblePixelWidth = sourceWidth * (ScrollArea.Bounds.Width / Image.Width);
        double visiblePixelHeight = sourceHeight * (ScrollArea.Bounds.Height / Image.Height);
        double maxPixelAxis = Math.Max(visiblePixelWidth, visiblePixelHeight);
        
        // If we're smaller than some fixed pixel count, do point sampling
        if (maxPixelAxis < 128)
        {
            RenderOptions.SetBitmapInterpolationMode(Image, BitmapInterpolationMode.None);
        }
        else
        {
            RenderOptions.SetBitmapInterpolationMode(Image, BitmapInterpolationMode.MediumQuality);
        }
    }

    /// <summary>
    /// Invoked on press events
    /// </summary>
    private void OnZoomPointerPressed(ImageWatchpointDisplayViewModel vm, PointerPressedEventArgs e)
    {
        if (vm.LockToContent)
        {
            vm.LockToContent = false;
        }
        
        if (!e.GetCurrentPoint(this).Properties.IsLeftButtonPressed)
        {
            return;
        }
                
        // Mark as dragging
        _isZoomDragging = true;
        _lastZoomDragPosition = e.GetPosition(this);
                
        // Capture the cursor
        Image.Cursor = new Cursor(StandardCursorType.SizeAll);
        e.Pointer.Capture(Image);
    }

    /// <summary>
    /// Invoked on release events
    /// </summary>
    private void OnZoomPointerReleased(ImageWatchpointDisplayViewModel vm, PointerReleasedEventArgs e)
    {
        if (!_isZoomDragging)
        {
            return;
        }
                
        // Release drag
        _isZoomDragging = false;
                
        // Release the cursor
        Image.Cursor = new Cursor(StandardCursorType.Arrow);
        e.Pointer.Capture(null);
    }

    /// <summary>
    /// Invoked on move events
    /// </summary>
    private void OnZoomPointerMoved(ImageWatchpointDisplayViewModel vm, PointerEventArgs e)
    {
        if (_isZoomDragging)
        {
            // Current offset
            Point position = e.GetPosition(this);
            Point delta = position - _lastZoomDragPosition;

            ScrollArea.Offset -= delta;

            // Pan the control
            _lastZoomDragPosition = position;
            return;
        }
        
        Point point = e.GetPosition(Image);

        // Get factors
        double factorX = point.X / Image.Bounds.Width;
        double factorY = point.Y / Image.Bounds.Height;

        // Get source coordinates
        uint imageX = (uint)((Image.Source?.Size.Width ?? 1) * factorX);
        uint imageY = (uint)((Image.Source?.Size.Height ?? 1) * factorY);

        // Let the inspector render the value
        PixelInspectionRender pixel = vm.Inspector?.Inspect(imageX, imageY) ?? new PixelInspectionRender()
        {
            Color = Colors.Transparent,
            NativeFormatRender = "Invalid"
        };

        // Update view model
        vm.PixelDecoration = $"Pixel X:{imageX} Y:{imageY} - {pixel.NativeFormatRender}";
        vm.PixelColor = new SolidColorBrush(pixel.Color);
    }

    /// <summary>
    /// Are we currently dragging the view?
    /// </summary>
    private bool _isZoomDragging = false;

    /// <summary>
    /// Cached width
    /// </summary>
    private int _lastSourceWidth = 0;
    
    /// <summary>
    /// Cached height
    /// </summary>
    private int _lastSourceHeight = 0;

    /// <summary>
    /// Current zoom factor
    /// </summary>
    private double _zoomFactor = 1.0f;

    /// <summary>
    /// Do we have a pending update?
    /// </summary>
    private bool _pendingLockUpdate = false;
        
    /// <summary>
    /// If dragging, the last position
    /// </summary>
    private Point _lastZoomDragPosition;
}