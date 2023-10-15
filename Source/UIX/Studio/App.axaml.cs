// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

using System;
using System.Globalization;
using System.Threading;
using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Avalonia.Markup.Xaml.Styling;
using Avalonia.Styling;
using LiveChartsCore;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.Themes;
using Studio.Plugin;
using Studio.Services;
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
            new StyleInclude(new Uri("avares://GPUReshape/Styles"))
            {
                Source = new Uri("avares://GPUReshape/Themes/DefaultStyle.axaml")
            }
        };

        public override void Initialize()
        {
            Styles.Insert(0, DefaultStyle);
            
            // Set culture
            Thread.CurrentThread.CurrentUICulture = CultureInfo.GetCultureInfo("en");

            // Install global services
            InstallServicesAndLoadPlugins();

            // Load app
            AvaloniaXamlLoader.Load(this);

            // Install additional packages
            InstallLiveChart();
        }
        
        private void InstallLiveChart()
        {
            LiveCharts.Configure(
                settings => settings
                    .AddDefaultMappers()
                    .AddSkiaSharp()
                    .AddDarkTheme(
                        theme =>
                            theme
                                .HasRuleForLineSeries(lineSeries => { lineSeries.LineSmoothness = 0.65; })
                                .HasRuleForBarSeries(barSeries => { })
                    ));
        }

        private void InstallServicesAndLoadPlugins()
        {
            AvaloniaLocator locator = AvaloniaLocator.CurrentMutable;
            
            // Attempt to find all plugins of relevance
            _pluginList = _pluginResolver.FindPlugins("uix", PluginResolveFlag.ContinueOnFailure);
            
            // Cold suspension service
            locator.BindToSelf<ISuspensionService>(new SuspensionService(System.IO.Path.Combine("Intermediate", "Settings", "Suspension.json")));
            
            // Locator
            locator.BindToSelf<ILocatorService>(new LocatorService());
            
            // Logging host
            locator.BindToSelf<ILoggingService>(new LoggingService());

            // Hosts all menu objects
            locator.BindToSelf<IWindowService>(new WindowService());

            // Hosts all live workspaces
            locator.BindToSelf<IWorkspaceService>(new WorkspaceService());
            
            // Provides general network diagnostics
            locator.BindToSelf(new NetworkDiagnosticService());

            // Initiates the host resolver if not already up and running
            locator.BindToSelf<IHostResolverService>(new HostResolverService());
            
            // Local discoverability
            locator.BindToSelf<IBackendDiscoveryService>(new BackendDiscoveryService());

            // Hosts all status objects
            locator.BindToSelf<IStatusService>(new StatusService());

            // Hosts all context objects
            locator.BindToSelf<IContextMenuService>(new ContextMenuService());

            // Hosts all menu objects
            locator.BindToSelf<IMenuService>(new MenuService());

            // Hosts all settings objects
            locator.BindToSelf<ISettingsService>(new SettingsService());
        }

        private void InstallPlugins()
        {
            // Install all plugins
            if (_pluginList != null)
            {
                _pluginResolver.InstallPlugins(_pluginList, PluginResolveFlag.ContinueOnFailure);
            }
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
                    var mainView = new MainView
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

        /// <summary>
        /// Shared plugin resolver
        /// </summary>
        private PluginResolver _pluginResolver = new();

        /// <summary>
        /// Resolved plugin list
        /// </summary>
        private PluginList? _pluginList;
    }
}