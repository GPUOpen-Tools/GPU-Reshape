using System;
using Avalonia.Controls;
using GRS.Features.Debug.UIX.ViewModels;
using ReactiveUI;
using Studio.Extensions;

namespace UIX.Views;

public partial class BreakpointDisplayView : UserControl, IViewFor
{
    public object? ViewModel
    {
        get => DataContext;
        set => DataContext = value;
    }

    public BreakpointDisplayView()
    {
        InitializeComponent();

        this.WhenAnyValue(x => x.DataContext)
            .CastNullable<BreakpointViewModel>()
            .Subscribe(x =>
            {
                StackView.DataContext = new BreakpointVariableViewModel()
                {
                    BreakpointViewModel = x
                };
            });
    }
}