using System;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Documents;

namespace Studio.Views.Documents
{
    public partial class WelcomeView : UserControl
    {
        public WelcomeView()
        {
            InitializeComponent();

            // Subscribe to events
            this.WhenAnyValue(x => x.DataContext).CastNullable<WelcomeViewModel>().Subscribe(x =>
            {
                x.OnClose.Subscribe(_ =>
                {
                    if (Avalonia.Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
                    {
                        desktop.MainWindow.Close();
                    }
                });
            });
        }
    }
}
