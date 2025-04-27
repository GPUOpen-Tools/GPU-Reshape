using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using DynamicData;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Studio.App.Commands.Render.Models;
using Studio.Models.Logging;
using Studio.Services;

namespace Studio.App.Commands.Render;

public class RenderReportModel
{
    /// <summary>
    /// General summary
    /// </summary>
    public RenderSummaryModel Summary { get; set; } = new();
    
    /// <summary>
    /// All logs, including modelling logs
    /// </summary>
    public List<RenderLogModel> Logs { get; set; } = new();

    /// <summary>
    /// All shaders with validation issues
    /// </summary>
    public List<RenderShaderModel> Shaders { get; set; } = new();

    public RenderReportModel(dynamic report)
    {
        // Render all logs before modelling the rest, since we may render additional logging
        ModelLogs(report);
        
        // Summarize the primary process
        ModelPrimaryDeviceSummary(report);

        // Visit and summarize/bucket the node hierarchy
        VisitProcessCollectionNode(report);
    }

    /// <summary>
    /// Modelize all logging
    /// </summary>
    private void ModelLogs(dynamic report)
    {
        foreach (dynamic log in report.Logs)
        {
            // Report all app logs as a "bootstrapper" event
            Logs.Add(new RenderLogModel
            {
                Severity = log.Severity,
                System = "Bootstrapper",
                Message = log.Message,
                ViewModel = log.ViewModel.ToObject<object>()
            });
            
            // Any VM?
            if (log.ViewModel == null)
            {
                continue;
            }

            // Only accepted view model is that of instrumentation, may change
            Summary.Instrumentation.PassedShaders += (int)log.ViewModel.PassedShaders;
            Summary.Instrumentation.PassedPipelines += (int)log.ViewModel.PassedPipelines;
            Summary.Instrumentation.FailedShaders += (int)log.ViewModel.FailedShaders;
            Summary.Instrumentation.FailedPipelines += (int)log.ViewModel.FailedPipelines;
            Summary.Instrumentation.ShaderSeconds += (int)log.ViewModel.ShaderMilliseconds / 1e3f;
            Summary.Instrumentation.PipelineSeconds += (int)log.ViewModel.PipelineMilliseconds / 1e3f;
        }
        
        // Subscribe to render events
        ServiceRegistry.Get<ILoggingService>()?.ViewModel.Events.Connect()
            .OnItemAdded(OnLogAdded)
            .Subscribe();
    }

    /// <summary>
    /// Invoked on model logs
    /// </summary>
    private void OnLogAdded(LogEvent obj)
    {
        // Report the added log as a "render" event
        Logs.Add(new RenderLogModel
        {
            Severity = obj.Severity.ToString(),
            System = "Render",
            Message = obj.Message
        });
    }

    /// <summary>
    /// Model the primary info
    /// </summary>
    private void ModelPrimaryDeviceSummary(dynamic report)
    {
        // Must have a root process
        if (GetRootProcess(report) is not {} process)
        {
            return;
        }
        
        // Find the first used device, an application may have multiple that are unused
        if (GetFirstActiveDevice(process.Value) is not {} device)
        {
            return;
        }
        
        // Set the primary info
        Summary.PrimaryProcessName = process.Name;
        Summary.PrimaryProcessConfiguration = device.Value.WorkspaceConfiguration;

        // Set the general configuration, if possible
        if (process.Value.Detail != null)
        {
            Summary.Detail = process.Value.Detail;
            Summary.Coverage = process.Value.Coverage;
            Summary.SynchronousRecording = process.Value.SynchronousRecording;
            Summary.TexelAddressing = process.Value.TexelAddressing;
            Summary.SafeGuard = process.Value.SafeGuard;
        }
    }

    /// <summary>
    /// Get the first used device
    /// </summary>
    private dynamic? GetFirstActiveDevice(dynamic process)
    {
        // If the configuration hasn't been set, it's not primary
        foreach (dynamic child in process.Devices)
        {
            if (!string.IsNullOrWhiteSpace((string)child.Value.WorkspaceConfiguration))
            {
                return child;
            }
        }

        return null;
    }

    /// <summary>
    /// Get the first process
    /// </summary>
    private dynamic? GetRootProcess(dynamic report)
    {
        foreach (dynamic child in report.Processes)
        {
            return child;
        }

        return null;
    }
    
    /// <summary>
    /// Visit a collection and its children
    /// </summary>
    private void VisitProcessCollectionNode(dynamic node)
    {
        foreach (dynamic child in node.Processes)
        {
            VisitProcessNode(child.Value);
        }
    }

    /// <summary>
    /// Visit a process and its children
    /// </summary>
    private void VisitProcessNode(dynamic process)
    {
        VisitProcessCollectionNode(process);
        
        foreach (dynamic device in process.Devices)
        {
            VisitDeviceNode(device.Value);
        }
    }

    /// <summary>
    /// Visit a device and its children
    /// </summary>
    private void VisitDeviceNode(dynamic child)
    {
        // Cleanup local cache
        _shaderLookup.Clear();

        // Process all shaders
        foreach (dynamic shader in child.Shaders)
        {
            ModelShader(shader);
        }
        
        // Process messages and their associations to shaders
        foreach (dynamic message in child.Messages)
        {
            ModelMessageSummary(message);
            ModelMessageShader(message);
        }
    }

    /// <summary>
    /// Model a shader
    /// </summary>
    private void ModelShader(dynamic shader)
    {
        // Create the model
        RenderShaderModel model = new();
        model.Model = shader;
        model.HasSymbols = shader.Files.Count > 0;

        // Append all files
        foreach (dynamic file in shader.Files)
        {
            model.Files.Add(new RenderShaderFileModel()
            {
                Model = file
            });
        }

        // OK
        _shaderLookup.Add((int)shader.GUID, model);
        Shaders.Add(model);
    }

    /// <summary>
    /// Model a message
    /// </summary>
    private void ModelMessageShader(dynamic message)
    {
        // Must exist
        if (!_shaderLookup.TryGetValue((int)message.Segment.SGUID, out RenderShaderModel? shaderModel))
        {
            Logging.Error($"Failed to associate message sguid {(int)message.Segment.SGUID}");
            return;
        }
        
        // Add to shader
        shaderModel.Messages.Add(message);
        shaderModel.MessageRenders.Add(RenderObject(message));

        // Valid symbol?
        if (message.Segment.FileUID >= shaderModel.Files.Count)
        {
            Logging.Error($"Invalid file uid in {(int)message.Segment.SGUID}");
            return;
        }

        // Add to shader file
        RenderShaderFileModel fileModel = shaderModel.Files[(int)message.Segment.FileUID];
        fileModel.Messages.Add(message);
    }

    /// <summary>
    /// Render an object for inlining
    /// </summary>
    private string RenderObject(dynamic _object)
    {
        switch (_object)
        {
            default:
            {
                Logging.Error("Invalid report data type");
                return "";
            }
            case JArray array:
            {
                StringBuilder builder = new();
                builder.Append('[');
                foreach (dynamic item in array)
                {
                    builder.Append(RenderObject(item));
                    builder.Append(", ");
                }
                builder.Append(']');
                return builder.ToString();
            }
            case JObject map:
            {
                StringBuilder builder = new();
                builder.Append('{');
                foreach (dynamic kv in map)
                {
                    builder.Append(kv.Key);
                    builder.Append(": ");
                    builder.Append(RenderObject(kv.Value));
                    builder.Append(", ");
                }
                builder.Append('}');
                return builder.ToString();
            }
            case JValue value:
            {
                switch (value.Value)
                {
                    default:
                        return value.Value?.ToString() ?? "null";
                    case string str:
                        return $"'{str}'";
                    case bool state:
                        return state ? "true" : "false";
                }
            }
        }
    }

    /// <summary>
    /// Model message summary
    /// </summary>
    private void ModelMessageSummary(dynamic message)
    {
        // Increment severity counters
        switch ((string)message.Severity)
        {
            case "Info":
                Summary.Message.Infos++;
                break;
            case "Warning":
                Summary.Message.Warnings++;
                break;
            case "Error":
                Summary.Message.Errors++;
                break;
        }
        
        // Get or create the chart item
        if (!_chartDataLookup.TryGetValue((string)message.Content, out RenderMessageChartDataModel? chartModel))
        {
            // Not found, create a new one
            chartModel = new();
            chartModel.Content = message.Content;
            chartModel.Color = GetChartColor((string)message.Severity);
            
            // OK
            Summary.Message.Chart.Data.Add(chartModel);
            _chartDataLookup.Add((string)message.Content, chartModel);
        }

        // We could the number of *unique* issues, not the exported count
        chartModel.Count++;
    }

    /// <summary>
    /// Get the chart item color
    /// </summary>
    private Color GetChartColor(string severity)
    {
        // Matched colors with rendered theme
        switch (severity)
        {
            default:
                return Color.Black;
            case "Info":
                return Color.FromArgb(124, 144, 154);
            case "Warning":
                return Color.FromArgb(219, 174, 90);
            case "Error":
                return Color.FromArgb(172, 62, 49);
        }
    }

    /// <summary>
    /// Create from string
    /// </summary>
    public static RenderReportModel? Deserialize(string contents)
    {
        // Just try to load it, assume invalid if failed
        try
        {
            if (JsonConvert.DeserializeObject(contents) is not {} report)
            {
                return null;
            }
            
            return new RenderReportModel(report);
        }
        catch (Exception e)
        {
            Logging.Error($"Failed to deserialize report data: {e}");
            return null;
        }
    }

    /// <summary>
    /// Create from file
    /// </summary>
    public static RenderReportModel? DeserializeFile(string path)
    {
        // Just try to load it, assume invalid if failed
        try
        {
            return Deserialize(File.ReadAllText(path));
        }
        catch (Exception e)
        {
            Logging.Error($"Failed to read report file: {e}");
            return null;
        }
    }

    /// <summary>
    /// Contents to chart data
    /// </summary>
    private Dictionary<string, RenderMessageChartDataModel> _chartDataLookup = new();
    
    /// <summary>
    /// GUID to shader
    /// </summary>
    private Dictionary<int, RenderShaderModel> _shaderLookup = new();
}
