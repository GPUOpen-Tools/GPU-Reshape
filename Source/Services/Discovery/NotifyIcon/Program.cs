using System;
using System.Threading;
using Avalonia;
using Avalonia.ReactiveUI;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;

namespace Studio
{
    class Program
    {
        public static GlobalFileLock Lock = new();
        
        [STAThread]
        private static void Main(string[] args)
        {
            // Attempt to acquire
            if (!Lock.Acquire())
            {
                return;
            }

            // Unique, start application
            BuildAvaloniaApp().StartWithClassicDesktopLifetime(args, ShutdownMode.OnExplicitShutdown);
        }
        
        public static AppBuilder BuildAvaloniaApp()
        {
            return AppBuilder.Configure<App>()
                .UseReactiveUI()
                .UsePlatformDetect()
                .LogToTrace();
        }
    }
}