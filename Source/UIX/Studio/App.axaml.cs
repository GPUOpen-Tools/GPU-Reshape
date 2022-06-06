using System;
using System.Globalization;
using System.Threading;
using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Avalonia.Markup.Xaml.Styling;
using Avalonia.Styling;
using Studio.ViewModels;
using Studio.Views;

namespace Studio
{
    public class App : Application
    {
        /// <summary>
        /// Default dark style
        /// </summary>
        public static readonly Styles DefaultStyle = new Styles
        {
            new StyleInclude(new Uri("avares://Studio/Styles"))
            {
                Source = new Uri("avares://Studio/Themes/DefaultStyle.axaml")
            }
        };

        public override void Initialize()
        {
            Styles.Insert(0, DefaultStyle);
            
            // Set culture
            Thread.CurrentThread.CurrentUICulture = CultureInfo.GetCultureInfo("en");

            // Load app
            AvaloniaXamlLoader.Load(this);
        }

        public override void OnFrameworkInitializationCompleted()
        {
            var mainWindowViewModel = new MainWindowViewModel();

            switch (ApplicationLifetime)
            {
                case IClassicDesktopStyleApplicationLifetime desktopLifetime:
                {
                    var mainWindow = new MainWindow
                    {
                        DataContext = mainWindowViewModel
                    };

                    // Bind close
                    mainWindow.Closing += (_, _) =>
                    {
                        mainWindowViewModel.CloseLayout();
                    };

                    // Set lifetime
                    desktopLifetime.MainWindow = mainWindow;

                    // Bind exit
                    desktopLifetime.Exit += (_, _) =>
                    {
                        mainWindowViewModel.CloseLayout();
                    };
                    break;
                }
                case ISingleViewApplicationLifetime singleViewLifetime:
                {
                    var mainView = new MainView()
                    {
                        DataContext = mainWindowViewModel
                    };

                    // Set lifetime
                    singleViewLifetime.MainView = mainView;
                    break;
                }
            }

            base.OnFrameworkInitializationCompleted();
        }
    }
}