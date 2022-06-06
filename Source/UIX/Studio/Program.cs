using System;
using Avalonia;
using Avalonia.ReactiveUI;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Projektanker.Icons.Avalonia;
using Projektanker.Icons.Avalonia.FontAwesome;

namespace Studio
{
    class Program
    {
        [STAThread]
        private static void Main(string[] args)
        {
            BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
        }
        
        public static AppBuilder BuildAvaloniaApp()
        {
            IconProvider.Register<FontAwesomeIconProvider>();
            
            return AppBuilder.Configure<App>()
                .UseReactiveUI()
                .UsePlatformDetect()
                .LogToTrace();
        }
    }
}