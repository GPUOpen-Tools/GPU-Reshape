using System;
using System.Collections.Generic;
using System.Linq;
using DynamicData;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Extensions;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.App.Commands.Cli;

public class CliWorkspaceConfiguration : IWorkspaceConfigurationViewModel
{
    /// <summary>
    /// Name of this configuration
    /// </summary>
    public string Name => "Headless Configuration";

    /// <summary>
    /// Flags, assume all
    /// </summary>
    public WorkspaceConfigurationFlag Flags =>
        WorkspaceConfigurationFlag.RequiresSynchronousRecording |
        WorkspaceConfigurationFlag.CanUseTexelAddressing |
        WorkspaceConfigurationFlag.CanSafeGuard |
        WorkspaceConfigurationFlag.CanDetail;

    /// <summary>
    /// All enabled features
    /// </summary>
    public string[] FeatureNames = Array.Empty<string>();

    /// <summary>
    /// Get the description for a message
    /// </summary>
    public string GetDescription(IWorkspaceViewModel workspaceViewModel)
    {
        // Match all enabled features
        IEnumerable<string>? features = workspaceViewModel.PropertyCollection
            .GetProperty<IFeatureCollectionViewModel>()?.Features
            .Select(x => GetCliName(x.Name))
            .Where(x => FeatureNames.Any(y => x.Equals(y, StringComparison.InvariantCultureIgnoreCase)));

        return features?.NaturalJoin() ?? "None";
    }

    /// <summary>
    /// Install this configuration
    /// </summary>
    public async void Install(IWorkspaceViewModel workspaceViewModel)
    {
        if (workspaceViewModel.PropertyCollection is not IInstrumentableObject instrumentable ||
            instrumentable.GetOrCreateInstrumentationProperty() is not { } propertyViewModel)
        {
            return;
        }

        // Get all active services
        IInstrumentationPropertyService[]? services = instrumentable.GetWorkspaceCollection()?.GetServices<IInstrumentationPropertyService>().ToArray();

        // Install against all services
        foreach (string featureName in FeatureNames)
        {
            // Find matching service
            if (services?.FirstOrDefault(x => GetCliName(x.Name).Equals(featureName, StringComparison.InvariantCultureIgnoreCase)) is not { } service)
            {
                Logging.Error($"Failed to find instrumentation service {featureName}, available feature set: {services?.Select(x => GetCliName(x.Name)).NaturalJoin() ?? "None"}");
                continue;
            }

            // Create feature
            if (await service.CreateInstrumentationObjectProperty(propertyViewModel, true) is { } instrumentationObjectProperty)
            {
                propertyViewModel.Properties.Add(instrumentationObjectProperty);
            }
        }
    }

    /// <summary>
    /// Get the CLI-friendly name of a feature
    /// </summary>
    private string GetCliName(string name)
    {
        return new string(name.Where(x => !char.IsWhiteSpace(x)).ToArray());
    }
}