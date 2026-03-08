using System;
using Avalonia.Controls;
using GRS.Features.Debug.UIX.ViewModels;
using ReactiveUI;
using Studio.Extensions;

namespace UIX.Views.Display;

public partial class LooseWatchpointDisplayView : UserControl, IViewFor
{
    public object? ViewModel
    {
        get => DataContext;
        set => DataContext = value;
    }

    public LooseWatchpointDisplayView()
    {
        InitializeComponent();
    }
}
