using System;
using System.Collections.Generic;
using System.Linq;
using Newtonsoft.Json;
using Runtime.ViewModels.Traits;
using Studio.Models.Workspace;
using Studio.Services;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.App.Commands.Export;

public class ExportReportModel
{
    /// <summary>
    /// All workspaces to export
    /// </summary>
    public IWorkspaceViewModel[]  Workspaces { get; set; }
    
    /// <summary>
    /// Configured workspaces were launched with
    /// </summary>
    public IWorkspaceConfigurationViewModel? WorkspaceConfiguration { get; set; }
    
    /// <summary>
    /// State all workspaces were launched with
    /// </summary>
    public WorkspaceLaunchState? LaunchState { get; set; }

    public bool WriteReport(string reportPath)
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
                reportPath,
                JsonConvert.SerializeObject(map, Formatting.Indented)
            );
        
            // Diagnostic
            Logging.Info($"Report serialized to '{reportPath}'");
            return true;
        }
        catch (Exception ex)
        {
            Logging.Error($"Failed to serialize report with: {ex}");
            return false;
        }
    }

    private void SerializeWorkspaces(SerializationMap map)
    {
        // Serialize all workspaces
        foreach (IWorkspaceViewModel workspaceViewModel in Workspaces)
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
                { "Detail", LaunchState?.Detail ?? false },
                { "Coverage", LaunchState?.Coverage ?? false },
                { "SynchronousRecording", LaunchState?.SynchronousRecording ?? false },
                { "TexelAddressing", LaunchState?.TexelAddressing ?? false },
                { "SafeGuard", LaunchState?.SafeGuard ?? false },
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
            { "WorkspaceConfiguration", WorkspaceConfiguration?.GetDescription(workspaceViewModel) ?? "Invalid" },
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
    /// The unique name counters
    /// </summary>
    private Dictionary<string, int> _namedCounter = new();
}
