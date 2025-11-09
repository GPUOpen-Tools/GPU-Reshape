using System;
using System.IO;
using Avalonia.Platform;
using Scriban;

namespace Studio.App.Commands.Render;

public class RenderReportTemplateModel
{
    /// <summary>
    /// Model to template render
    /// </summary>
    public RenderReportModel Model { get; set; }

    /// <summary>
    /// Render the contents
    /// </summary>
    public bool Render(string path)
    {
        string outExt = Path.GetExtension(path).ToLowerInvariant();
        
        // Render contents
        string? contents;
        switch (outExt)
        {
            default:
                Logging.Error($"Unsupported file extension {outExt}, must be one of [html]");
                return false;
            case ".html":
                contents = RenderHtml(Model);
                break;
        }
        
        // Try to write contents
        try
        {
            File.WriteAllText(path, contents);
            Logging.Info($"Render serialized to '{path}'");
        }
        catch
        {
            Logging.Error("Failed to write rendered report");
            return false;
        }
        
        // OK
        return true;
    }

    /// <summary>
    /// Render the html contents
    /// </summary>
    private string? RenderHtml(RenderReportModel model)
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
}
