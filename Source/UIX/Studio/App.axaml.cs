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
        // Default locator
        public static IAvaloniaDependencyResolver Locator = AvaloniaLocator.Current;
        
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

        private void InstallServices()
        {
            AvaloniaLocator locator = AvaloniaLocator.CurrentMutable;
            
            // Logging host
            locator.BindToSelf<Services.ILoggingService>(new Services.LoggingService());

            // Initiates the host resolver if not already up and running
            locator.BindToSelf<Services.IHostResolverService>(new Services.HostResolverService());

            // Hosts all live workspaces
            locator.BindToSelf<Services.IWorkspaceService>(new Services.WorkspaceService());
        }
        
        public override void OnFrameworkInitializationCompleted()
        {
            // Install global services
            InstallServices();

            // Create view model
            var vm = new MainWindowViewModel();

            // Bind lifetime
            switch (ApplicationLifetime)
            {
                case IClassicDesktopStyleApplicationLifetime desktopLifetime:
                {
                    var mainWindow = new MainWindow
                    {
                        DataContext = vm
                    };

                    // Bind close
                    mainWindow.Closing += (_, _) =>
                    {
                        vm.CloseLayout();
                    };

                    // Set lifetime
                    desktopLifetime.MainWindow = mainWindow;

                    // Bind exit
                    desktopLifetime.Exit += (_, _) =>
                    {
                        vm.CloseLayout();
                    };
                    break;
                }
                case ISingleViewApplicationLifetime singleViewLifetime:
                {
                    var mainView = new MainView()
                    {
                        DataContext = vm
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