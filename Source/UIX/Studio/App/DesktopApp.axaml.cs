// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
using System.CommandLine;
using System.Globalization;
using System.Threading;
using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Avalonia.Markup.Xaml.Styling;
using Avalonia.ReactiveUI;
using Avalonia.Styling;
using LiveChartsCore;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.Themes;
using Projektanker.Icons.Avalonia;
using Projektanker.Icons.Avalonia.FontAwesome;
using Studio.App.Commands;
using Studio.ViewModels;
using Studio.Views;

namespace Studio.App
{
    public class DesktopApp : Application
    {
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

        // Setup command signatures
        static readonly RootCommand Command = new RootCommand("GPU Reshape")
        {
            AttachCommand.Create()
        };

        public static void Build(string[] args)
        {
            BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
        }
        
        public static AppBuilder BuildAvaloniaApp()
        {
            IconProvider.Current.Register<FontAwesomeIconProvider>();
            
            return AppBuilder.Configure<DesktopApp>()
                .UseReactiveUI()
                .UsePlatformDetect()
                .LogToTrace();
        }

        public override void Initialize()
        {
            Styles.Insert(0, DefaultStyle);
            
            // Set culture
            Thread.CurrentThread.CurrentUICulture = CultureInfo.GetCultureInfo("en");

            // Install global services
            _serviceProvider.Install();
            
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
        
        public override async void OnFrameworkInitializationCompleted()
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
            _serviceProvider.InstallPlugins();

            // Invoke command line if requested
            if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime { Args: { Length: > 0 } args })
            {
                await Command.InvokeAsync(args);
            }
            
            base.OnFrameworkInitializationCompleted();
        }

        /// <summary>
        /// Internal service provider
        /// </summary>
        private ServiceProvider _serviceProvider = new();
    }
}