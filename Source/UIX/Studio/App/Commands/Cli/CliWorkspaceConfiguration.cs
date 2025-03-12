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
            .Select(x => x.Name)
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
            if (services?.FirstOrDefault(x => x.Name.Equals(featureName, StringComparison.InvariantCultureIgnoreCase)) is not { } service)
            {
                Logging.Error($"Failed to find instrumentation service {featureName}, available feature set: {services?.Select(x => x.Name).NaturalJoin() ?? "None"}");
                continue;
            }

            // Create feature
            if (await service.CreateInstrumentationObjectProperty(propertyViewModel, true) is { } instrumentationObjectProperty)
            {
                propertyViewModel.Properties.Add(instrumentationObjectProperty);
            }
        }
    }
}