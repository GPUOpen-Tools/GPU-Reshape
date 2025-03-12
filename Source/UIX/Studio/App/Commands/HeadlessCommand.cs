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
using System.Collections.Generic;
using System.CommandLine;
using System.CommandLine.Invocation;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Discovery.CLR;
using DynamicData;
using Message.CLR;
using Newtonsoft.Json;
using Runtime.ViewModels.Traits;
using Studio.App.Commands.Cli;
using Studio.Models.Workspace;
using Studio.Services;
using Studio.ViewModels;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Formatting = Newtonsoft.Json.Formatting;

namespace Studio.App.Commands;

public class HeadlessCommand : IBaseCommand
{
    /// <summary>
    /// Create the headless command
    /// </summary>
    public static Command Create()
    {
        // Setup command
        return new Command("headless").Make(new HeadlessCommand(), new Option[]
        {
            App,
            WorkingDirectory,
            OutReport,
            Workspace,
            Timeout
        });
    }

    static HeadlessCommand()
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
            CancellationToken token = CancellationToken.None;

            // Assign timeout token if needed
            if (context.ParseResult.GetValueForOption(Timeout) is { } timeout)
            {
                token = new CancellationTokenSource(TimeSpan.FromSeconds(timeout)).Token;
            }
            
            try
            {
                await process.WaitForExitAsync(token);
            }
            catch (OperationCanceledException)
            {
                Logging.Error("Wait for process termination timed out");
                process.Kill();
            }
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
    /// Write/serialize the report
    /// </summary>
    private bool WriteReport(InvocationContext context)
    {
        // Serialize all processes (root is treated as a workspace)
        SerializationMap processes = new();
        SerializeWorkspaces(processes);

        // Create root object
        SerializationMap map = new()
        {
            { "Processes", processes },
            {
                "Logs",
                ServiceRegistry.Get<ILoggingService>()?.ViewModel.Events.Items.Select(x =>
                {
                    return new SerializationMap()
                    {
                        { "Severity", Enum.GetName(x.Severity) },
                        { "Message", x.Message },
                        {
                            "ViewModel",
                            (x.ViewModel as ISerializable)?.Serialize()
                        }
                    };
                })
            }
        };
            
        try
        {
            // Try to serialize the report
            System.IO.File.WriteAllText(
                context.ParseResult.GetValueForOption(OutReport)!,
                JsonConvert.SerializeObject(map, Formatting.Indented)
            );
            
            return true;
        }
        catch (Exception ex)
        {
            Logging.Error($"Failed to serialize report with: {ex}");
            return false;
        }
    }

    /// <summary>
    /// Serialize a workspace
    /// </summary>
    /// <param name="map"></param>
    private void SerializeWorkspaces(SerializationMap map)
    {
        if (ServiceRegistry.Get<IWorkspaceService>() is not { } workspaceService)
        {
            return;
        }
        
        // Serialize all workspaces
        foreach (IWorkspaceViewModel workspaceViewModel in workspaceService.Workspaces.Items)
        {
            SerializeWorkspace(workspaceViewModel, map);
        }
    }

    /// <summary>
    /// Serialize a workspace
    /// </summary>
    private void SerializeWorkspace(IWorkspaceViewModel workspaceViewModel, SerializationMap map)
    {
        // Serialize sub-maps
        SerializeProperties(
            workspaceViewModel.PropertyCollection,
            out SerializationMap processes,
            out SerializationMap devices
        );

        // If a process collection, just report the processes
        if (workspaceViewModel is ProcessWorkspaceViewModel)
        {
            map.Add(GetUniqueName(workspaceViewModel.Connection?.Application?.DecoratedName ?? "Unknown"), new SerializationMap()
            {
                { "CommandLine", string.Join(" ", Environment.GetCommandLineArgs()) },
                { "Detail", _launchViewModel.Detail },
                { "Coverage", _launchViewModel.Coverage },
                { "SynchronousRecording", _launchViewModel.SynchronousRecording },
                { "TexelAddressing", _launchViewModel.TexelAddressing },
                { "SafeGuard", _launchViewModel.SafeGuard },
                { "Processes", processes },
                { "Devices", devices }
            });
            return;
        }

        // All referenced shader guids
        HashSet<ulong> shadersGuids = new();

        // Serialize all messages
        List<object> messageMap = new();
        if (workspaceViewModel.PropertyCollection.GetProperty<MessageCollectionViewModel>() is { } messageCollection)
        {
            foreach (ValidationObject validationObject in messageCollection.ValidationObjects)
            {
                // Collect shader guids if referenced
                if (validationObject.Segment is { } segment)
                {
                    shadersGuids.Add(segment.Location.SGUID);
                }
                
                messageMap.Add(validationObject.Serialize());
            }
        }

        // Serialize all referenced shaders
        List<object> shaders = new();
        if (workspaceViewModel.PropertyCollection.GetProperty<ShaderCollectionViewModel>() is { } shaderCollection)
        {
            foreach (ulong shader in shadersGuids)
            {
                if (shaderCollection.GetShader(shader) is { } shaderObject)
                {
                    shaders.Add(shaderObject.Serialize());
                }
            }
        }

        // Create map
        map.Add(GetUniqueName(workspaceViewModel.Connection?.Application?.DecoratedName ?? "Unknown"), new SerializationMap()
        {
            { "Application", workspaceViewModel.Connection?.Application },
            { "WorkspaceConfiguration", _workspaceConfiguration?.GetDescription(workspaceViewModel) ?? "Invalid" },
            { "Messages", messageMap },
            { "Shaders", shaders }
        });
    }

    /// <summary>
    /// Serialize all properties
    /// </summary>
    private void SerializeProperties(IPropertyViewModel propertyViewModel, out SerializationMap processMap, out SerializationMap deviceMap)
    {
        processMap = new SerializationMap();
        deviceMap = new SerializationMap();
        
        // Serialize all workspace properties
        foreach (IPropertyViewModel childPropertyViewModel in propertyViewModel.Properties.Items)
        {
            switch (childPropertyViewModel)
            {
                case ProcessNodeViewModel node:
                {
                    if (node.GetWorkspace() is { } workspace)
                    {
                        SerializeWorkspace(workspace, processMap);
                    }
                    break;
                }
                case WorkspaceCollectionViewModel workspaceViewModel:
                {
                    SerializeWorkspace(workspaceViewModel.WorkspaceViewModel, deviceMap);
                    break;
                }
            }
        }
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
            Arguments = string.Join(" ", new ArraySegment<string?>(appAndArguments, 1, appAndArguments.Length - 1)),
            SelectedConfiguration = _workspaceConfiguration,
            AttachAllDevices = true,
            CaptureChildProcesses = true,
            Coverage = userWorkspace.Config.Coverage,
            Detail = userWorkspace.Config.Detail
            // TODO: Wait for connection tag
        };

        // Always accept
        _launchViewModel.AcceptLaunch.RegisterHandler(ctx => ctx.SetOutput(true));

        // Launch the application
        _launchViewModel.Start.Execute(null);
        
        // Check result
        if (_launchViewModel.ConnectionStatus == ConnectionStatus.FailedLaunch)
        {
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
    /// Get a unique name
    /// </summary>
    private string GetUniqueName(string name)
    {
        // First instance?
        if (_namedCounter.TryAdd(name, 1))
        {
            return name;
        }

        // Get unique name
        return $"{name}_{_namedCounter[name]++}";
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

    /// <summary>
    /// The unique name counters
    /// </summary>
    private Dictionary<string, int> _namedCounter = new();
}