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
using System.Threading;
using Avalonia;
using Avalonia.Threading;
using DynamicData;
using Studio.App.Commands;
using Studio.Models.Logging;
using Studio.Services;

namespace Studio.App;

public class CliApp : Application
{
    // Setup command signatures
    static readonly RootCommand Command = new RootCommand("GPU Reshape")
    {
        LaunchCommand.Create(),
        RenderCommand.Create()
    };
    
    public static void Build(string[] args)
    {
        AppBuilder.Configure<CliApp>()
            .UsePlatformDetect()
            .LogToTrace()
            .Start(AppMain, args);
    }

    public static bool IsCLI(string[] args)
    {
        return Command.Parse(args).Errors.Count == 0;
    }

    private static void AppMain(Application app, string[] args)
    {
        // Invoke the handler on the UI thread, then run the dispatcher on the current thread
        // Workspace management heavily relies on async dispatching, so we need to start it
        Dispatcher.UIThread.InvokeAsync(() => AppMainTask(args));
        Dispatcher.UIThread.MainLoop(_cancellationToken.Token);
    }

    private static async void AppMainTask(string[] args)
    {
        // Invoke sync
        await Command.InvokeAsync(args);

        // Stop the dispatcher
        _cancellationToken.Cancel();
    }

    public override void Initialize()
    {
        // Install plugins
        ServiceProvider provider = new();
        provider.Install();
        provider.InstallPlugins();
        
        // Attach all logging events
        if (ServiceRegistry.Get<ILoggingService>() is { } log)
        {
            log.ViewModel.Events.Connect()
                .OnItemAdded(OnLogAdded)
                .Subscribe();
        }
    }
    
    /// <summary>
    /// Invoked on log events
    /// </summary>
    private static void OnLogAdded(LogEvent log)
    {
        // Just print to the console
        Console.WriteLine($"{DateTime.Now:hh:mm:ss} [{log.Severity}] {log.Message}");
    }

    /// <summary>
    /// Dispatcher cancellation token
    /// </summary>
    private static CancellationTokenSource _cancellationToken = new();
}