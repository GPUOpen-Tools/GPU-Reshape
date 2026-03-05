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
using System.CommandLine.Invocation;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Discovery.CLR;
using DynamicData;
using Message.CLR;
using Studio.App.Commands.Cli;
using Studio.App.Commands.Export;
using Studio.Models.Workspace;
using Studio.Platform;
using Studio.Services;
using Studio.ViewModels;
using Studio.ViewModels.Setting;
using Studio.ViewModels.Workspace;

namespace Studio.App.Commands;

public class LaunchCommand : IBaseCommand
{
    /// <summary>
    /// Create the headless command
    /// </summary>
    public static Command Create()
    {
        // Setup command
        return new Command("launch").Make(new LaunchCommand(), new Option[]
        {
            App,
            WorkingDirectory,
            OutReport,
            Workspace,
            SymbolPath,
            Timeout
        });
    }

    static LaunchCommand()
    {
        // Let the launcher set up the wd
        WorkingDirectory.SetDefaultValue("");
    }

    /// <summary>
    /// Command invocation
    /// </summary>
    public async Task<int> InvokeAsync(InvocationContext context)
    {
        // Attach all workspace events
        AttachWorkspaces();

        // Try to launch
        if (LaunchApplication(context) is not {} processInfo)
        {
            return 1;
        }
        
        // Wait for the process to finish
        if (Process.GetProcessById((int)processInfo.processId) is {} process)
        {
            // If there's a pipe, handle that
            if (processInfo.writePipe != 0)
            {
                await WaitAndRedirectProcessPipe(context, process, processInfo);
            }
            else
            {
                WaitForProcess(context, process);
            }
        }
        else
        {
            Logging.Info("Process exited");
        }

        // Finally, write the report
        if (!WriteReport(context))
        {
            return 1;
        }
        
        // OK
        return 0;
    }

    /// <summary>
    /// Wait for a process to complete, with timeout
    /// </summary>
    private async void WaitForProcess(InvocationContext context, Process process)
    {
        CancellationToken token = CancellationToken.None;

        // Assign timeout token if needed
        if (context.ParseResult.GetValueForOption(Timeout) is { } timeout)
        {
            token = new CancellationTokenSource(TimeSpan.FromSeconds(timeout)).Token;
        }

        try
        {
            await process.WaitForExitAsync(token);
            Logging.Info($"Process exited with {process.ExitCode}");
        }
        catch (OperationCanceledException)
        {
            Logging.Error("Wait for process termination timed out");
            process.Kill();
        }
    }

    /// <summary>
    /// Wait for a process to complete and redirect its contents, with timeout
    /// </summary>
    private async Task WaitAndRedirectProcessPipe(InvocationContext context, Process process, DiscoveryProcessInfo processInfo)
    {
        // All timeouts are optional
        int? timeout = context.ParseResult.GetValueForOption(Timeout);

        // Get the handle
        // Do this before waiting on the process
        IntPtr processHandle;
        try
        {
            processHandle = process.Handle;
        }
        catch (Exception)
        {
            Logging.Error("Failed to get process handle");
            return;
        }
        
        // Watch for timeouts
        Stopwatch stopwatch = Stopwatch.StartNew();
        
        // Local buffer for write pipe
        byte[] buffer = new byte[4096];

        // While alive
        while (!process.HasExited)
        {
            // Check timeout
            if (timeout != null && stopwatch.Elapsed.Seconds >= timeout.Value)
            {
                Logging.Error("Wait for process termination timed out");
                process.Kill();
                return;
            }

            // Number of bytes read from the pipe
            uint bytesRead = 0;
            
            // Split out unsafe, can't yield to other tasks in here
            bool yieldInManaged = false;
            unsafe
            {
                IntPtr readPipeHandle = new((void*)processInfo.readPipe);
            
                // Any data in the pipe?
                // If so, try to read it
                uint bytesAvailable;
                if (!Win32.PeekNamedPipe(readPipeHandle, buffer, 0, out _, out bytesAvailable, out _) || bytesAvailable == 0 ||
                    !Win32.ReadFile(readPipeHandle, buffer, (uint)buffer.Length, out bytesRead, IntPtr.Zero) || bytesRead == 0)
                {
                    yieldInManaged = true;
                }
            }

            // Yield on a manged context
            // Let the other workspace tasks take over for a bit
            if (yieldInManaged)
            {
                await Task.Delay(TimeSpan.FromMilliseconds(100));
                continue;
            }
            
            // Redirect to this process
            Console.Write(Encoding.Default.GetString(buffer, 0, (int)bytesRead));
        }

        // Try to get exit code, use interop due to odd managed behaviour with process ownership
        if (!Win32.GetExitCodeProcess(processHandle, out uint exitCode))
        {
            Logging.Error("Failed to get process exit code");
            return;
        }
        
        // Done
        Logging.Info($"Process exited with {exitCode}");
    }

    /// <summary>
    /// Write/serialize the report
    /// </summary>
    private bool WriteReport(InvocationContext context)
    {
        if (ServiceRegistry.Get<IWorkspaceService>() is not { } workspaceService)
        {
            return false;
        }

        // Create model
        ExportReportModel model = new()
        {
            Workspaces = workspaceService.Workspaces.Items.ToArray(),
            WorkspaceConfiguration = _workspaceConfiguration,
            LaunchState = _launchViewModel.CreateLaunchState()
        };
        
        return model.WriteReport(context.ParseResult.GetValueForOption(OutReport)!);
    }

    /// <summary>
    /// Create the workspace settings and general environment
    /// </summary>
    private void CreateWorkspaceSettings(InvocationContext context, string process)
    {
        // Symbol path is optional
        if (context.ParseResult.GetValueForOption(SymbolPath) is not { } path)
        {
            return;
        }
        
        if (ServiceRegistry.Get<ISettingsService>() is not { ViewModel: { } settingViewModel })
        {
            Logging.Error("Failed to get settings service");
            return;
        }
        
        // Get application settings
        ApplicationSettingViewModel appSettings = settingViewModel
            .GetItemOrAdd<ApplicationListSettingViewModel>()
            .GetProcessOrAdd(process);

        // Configure symbol search path
        var pdbSettings = appSettings.GetItemOrAdd<PDBSettingViewModel>();
        pdbSettings.SearchDirectories.Clear();
        pdbSettings.SearchDirectories.Add(path);
        pdbSettings.SearchInSubFolders = true;
        
        // Diagnostic
        Logging.Info($"Mounting symbol path for {path}");
    }

    /// <summary>
    /// Launch an application
    /// </summary>
    private DiscoveryProcessInfo? LaunchApplication(InvocationContext context)
    {
        // Get arguments
        string[] appAndArguments = context.ParseResult.GetValueForOption(App)!;

        // Try to load workspace
        if (CliUserWorkspace.DeserializeFile(context.ParseResult.GetValueForOption(Workspace)!) is not {} userWorkspace)
        {
            return null;
        }

        // Create settings
        CreateWorkspaceSettings(context, Path.GetFileName(appAndArguments[0]));
        
        // Serialize all settings before launching
        if (ServiceRegistry.Get<ISuspensionService>() is { } suspensionService)
        {
            suspensionService.Suspend();
        }
        else
        {
            Logging.Error("Failed to suspend settings");
        }

        // Join arguments
        string argumentString = string.Join(" ", new ArraySegment<string?>(appAndArguments, 1, appAndArguments.Length - 1));
        Logging.Info($"Launching '{appAndArguments[0]} {argumentString}'");

        // Select configuration
        _workspaceConfiguration = new CliWorkspaceConfiguration()
        {
            FeatureNames = userWorkspace.Features
        };

        // Setup launch
        _launchViewModel = new()
        {
            ApplicationPath = appAndArguments[0],
            WorkingDirectoryPath = context.ParseResult.GetValueForOption(WorkingDirectory)!,
            Arguments = argumentString,
            SelectedConfiguration = _workspaceConfiguration,
            AttachAllDevices = true,
            CaptureChildProcesses = true,
            RedirectPipes = userWorkspace.Config.RedirectOutput,
            SuspendDeferredInitialization = userWorkspace.Config.SuspendDeferredInitialization,
            WaitForDebugger = userWorkspace.Config.WaitForDebugger,
            Coverage = userWorkspace.Config.Coverage,
            Detail = userWorkspace.Config.Detail
            // TODO: Wait for connection tag
        };

        // Always use external references on shaders to make sure we can query after app-side releases
        var useExternalRef = _launchViewModel.MessageEnvironmentView.Add<SetUseShaderExternalReferenceMessage>();
        useExternalRef.enabled = 1;

        // Always accept
        _launchViewModel.AcceptLaunch.RegisterHandler(ctx => ctx.SetOutput(true));

        // Launch the application
        _launchViewModel.Start.Execute(null);
        
        // Check result
        if (_launchViewModel.ConnectionStatus == ConnectionStatus.FailedLaunch)
        {
            Logging.Error("Failed to launch application");
            return null;
        }

        // OK
        return _launchViewModel.DiscoveryProcessInfo;
    }

    /// <summary>
    /// Attach workspace creation
    /// </summary>
    private void AttachWorkspaces()
    {
        // Attach workspace events
        if (ServiceRegistry.Get<IWorkspaceService>() is { } workspaceService)
        {
            workspaceService.Workspaces.Connect()
                .OnItemAdded(OnWorkspaceAdded)
                .Subscribe();
            
            // Add local extension
            workspaceService.Extensions.Add(new CliWorkspaceExtension());
        }
    }

    /// <summary>
    /// Invoked on workspace creation
    /// </summary>
    private void OnWorkspaceAdded(IWorkspaceViewModel workspace)
    {
        // Mark the workspace as acquired
        if (workspace.Connection?.GetSharedBus() is { } bus)
        {
            var msg = bus.Add<HeadlessWorkspaceReadyMessage>();
            msg.acquiredDeviceUid = (uint)(workspace.Connection.Application?.DeviceUid ?? 0);
        }
    }

    /// <summary>
    /// Application option
    /// </summary>
    private static readonly Option<string[]> App = new("-app", "The application to run with its arguments")
    {
        IsRequired = true,
        AllowMultipleArgumentsPerToken = true
    };
    
    /// <summary>
    /// Report file option
    /// </summary>
    private static readonly Option<string> OutReport = new("-report", "The output report path (json)")
    {
        IsRequired = true
    };
    
    /// <summary>
    /// Workspace file option
    /// </summary>
    private static readonly Option<string> Workspace = new("-workspace", "The workspace path (json)")
    {
        IsRequired = true
    };
    
    /// <summary>
    /// Working directory option
    /// </summary>
    private static readonly Option<string> WorkingDirectory = new("-wd", "The app working directory");
    
    /// <summary>
    /// Symbol directory
    /// </summary>
    private static readonly Option<string> SymbolPath = new("-symbol", "The symbol directory");
    
    /// <summary>
    /// Timeout option
    /// </summary>
    private static readonly Option<int?> Timeout = new("-timeout", "Timeout in seconds");
    
    /// <summary>
    /// The selected configuration
    /// </summary>
    private IWorkspaceConfigurationViewModel? _workspaceConfiguration;
    
    /// <summary>
    /// The underlying launcher
    /// </summary>
    private LaunchViewModel _launchViewModel;
}