using System;
using System.Globalization;
using System.Threading;
using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Avalonia.Markup.Xaml.Styling;
using Avalonia.Styling;
using ReactiveUI;
using Studio.Plugin;
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

            // Install global services
            InstallServices();

            // Load app
            AvaloniaXamlLoader.Load(this);
        }

        private void InstallServices()
        {
            AvaloniaLocator locator = AvaloniaLocator.CurrentMutable;
            
            // Logging host
            locator.BindToSelf<Services.ILoggingService>(new Services.LoggingService());

            // Hosts all menu objects
            locator.BindToSelf<Services.IWindowService>(new Services.WindowService());

            // Hosts all live workspaces
            locator.BindToSelf<Services.IWorkspaceService>(new Services.WorkspaceService());

            // Hosts all status objects
            locator.BindToSelf<Services.IStatusService>(new Services.StatusService());

            // Hosts all context objects
            locator.BindToSelf<Services.IContextMenuService>(new Services.ContextMenuService());

            // Hosts all menu objects
            locator.BindToSelf<Services.IMenuService>(new Services.MenuService());

            // Initiates the host resolver if not already up and running
            locator.BindToSelf<Services.IHostResolverService>(new Services.HostResolverService());
        }

        private void InstallPlugins()
        {
            PluginResolver resolver = new();

            // Attempt to find all plugins of relevance
            PluginList? list = resolver.FindPlugins("uix", PluginResolveFlag.ContinueOnFailure);
            if (list == null)
                return;

            // Install all plugins
            resolver.InstallPlugins(list, PluginResolveFlag.ContinueOnFailure);
        }
        
        public override void OnFrameworkInitializationCompleted()
        {
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
                        vm.CloseLayout.Execute(null);
                    };

                    // Set lifetime
                    desktopLifetime.MainWindow = mainWindow;

                    // Bind exit
                    desktopLifetime.Exit += (_, _) =>
                    {
                        vm.CloseLayout.Execute(null);
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
            
            // Install all user plugins
            InstallPlugins();

            base.OnFrameworkInitializationCompleted();
        }
    }
}