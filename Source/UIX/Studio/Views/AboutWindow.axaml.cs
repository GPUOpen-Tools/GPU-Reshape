using System;
using System.Reactive;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using DynamicData.Binding;
using ReactiveUI;
using Studio.Extensions;

namespace Studio.Views
{
    public partial class AboutWindow : Window
    {
        public AboutWindow()
        {
            InitializeComponent();
#if DEBUG
            this.AttachDevTools();
#endif

            // Bind
            CloseButton.Events().Click.Subscribe(_ => Close());
        }
    }
}