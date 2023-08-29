// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

using System.Collections.Generic;
using System.Linq;
using DynamicData;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Extensions;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace.Configurations
{
    public class AllConfigurationViewModel : IWorkspaceConfigurationViewModel
    {
        /// <summary>
        /// Name of this configuration
        /// </summary>
        public string Name { get; } = Resources.Resources.Workspace_Configuration_All_Name;

        /// <summary>
        /// Can this configuration safe guard?
        /// </summary>
        public bool CanSafeGuard => true;

        /// <summary>
        /// Does this configuration require safe guarding?
        /// </summary>
        public bool RequiresSynchronousRecording => true;

        /// <summary>
        /// Get the description for a message
        /// </summary>
        public string GetDescription(IWorkspaceViewModel workspaceViewModel)
        {
            IEnumerable<string>? features = workspaceViewModel.PropertyCollection
                .GetProperty<IFeatureCollectionViewModel>()?.Features
                .Select(x => x.Name);
                
            return $"{Resources.Resources.Workspace_Configuration_All_Header} {features?.NaturalJoin() ?? "None"}";
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
            
            // Create all instrumentation properties
            foreach (IInstrumentationPropertyService service in instrumentable.GetWorkspace()?.GetServices<IInstrumentationPropertyService>() ?? Enumerable.Empty<IInstrumentationPropertyService>())
            {
                if (await service.CreateInstrumentationObjectProperty(propertyViewModel, true) is { } instrumentationObjectProperty)
                {
                    propertyViewModel.Properties.Add(instrumentationObjectProperty);
                }
            }
        }
    }
}