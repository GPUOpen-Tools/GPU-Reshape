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
using System.IO;
using System.Threading.Tasks;
using Avalonia.Platform;
using Scriban;
using Studio.App.Commands.Render;

namespace Studio.App.Commands;

public class RenderCommand : IBaseCommand
{
    /// <summary>
    /// Create the render command
    /// </summary>
    public static Command Create()
    {
        // Setup command
        return new Command("render").Make(new RenderCommand(), new Option[]
        {
            Report,
            Out
        });
    }

    static RenderCommand()
    {
        
    }

    /// <summary>
    /// Command invocation
    /// </summary>
    public async Task<int> InvokeAsync(InvocationContext context)
    {
        string outPath = context.ParseResult.GetValueForOption(Out)!;
        string outExt = Path.GetExtension(outPath).ToLowerInvariant();

        // Deserialize and model the report
        if (RenderReportModel.DeserializeFile(context.ParseResult.GetValueForOption(Report)!) is not { } model)
        {
            return 1;
        }

        // Render contents
        string? contents;
        switch (outExt)
        {
            default:
                Logging.Error($"Unsupported file extension {outExt}, must be one of [html]");
                return 1;
            case ".html":
                contents = RenderHtml(context, model);
                break;
        }
        
        // Try to write contents
        try
        {
            File.WriteAllText(outPath, contents);
            Logging.Info($"Render serialized to '{outPath}'");
        }
        catch
        {
            Logging.Error("Failed to write rendered report");
            return 1;
        }
        
        // OK
        return 0;
    }

    /// <summary>
    /// Render the html contents
    /// </summary>
    private string? RenderHtml(InvocationContext context, RenderReportModel model)
    {
        // Load embedded template
        if (LoadTemplateContents("StaticTemplate.scriban.html") is not { } templateContents)
        {
            return null;
        }

        // Try to parse the template
        if (Template.Parse(templateContents) is not { } template)
        {
            return null;
        }
        
        // Try to render the template
        try
        {
            return template.Render(new
            {
                Model = model
            }, m => m.Name);
        }
        catch (Exception e)
        {
            Logging.Error($"Failed to render template: {e}");
            return null;
        }
    }

    /// <summary>
    /// Load a template
    /// </summary>
    private string? LoadTemplateContents(string templateName)
    {
        // All templates are embedded resources
        if (AssetLoader.Open(new Uri($"avares://GPUReshape/Resources/Render/{templateName}")) is not {} templateStream)
        {
            Logging.Error("Failed to open template");
            return null;
        }

        using var reader = new StreamReader(templateStream);
        return reader.ReadToEnd();
    }

    /// <summary>
    /// Report file option
    /// </summary>
    private static readonly Option<string> Report = new("-report", "The input report path (json)")
    {
        IsRequired = true
    };
    
    /// <summary>
    /// Output file option
    /// </summary>
    private static readonly Option<string> Out = new("-out", "The output path (must include extension, dictates render mode [html])")
    {
        IsRequired = true
    };
}