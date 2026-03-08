using System;
using Avalonia.Controls;
using GRS.Features.Debug.UIX.ViewModels;
using ReactiveUI;
using Studio.Extensions;

namespace UIX.Views;

public partial class WatchpointDisplayView : UserControl, IViewFor
{
    public object? ViewModel
    {
        get => DataContext;
        set => DataContext = value;
    }

    public WatchpointDisplayView()
    {
        InitializeComponent();

        this.WhenAnyValue(x => x.DataContext)
            .CastNullable<WatchpointViewModel>()
            .Subscribe(x =>
            {
                StackView.DataContext = new WatchpointVariableViewModel()
                {
                    WatchpointViewModel = x
                };
            });
    }
}