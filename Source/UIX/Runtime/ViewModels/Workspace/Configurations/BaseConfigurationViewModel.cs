﻿// 
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

using DynamicData;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Workspace;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace.Configurations
{
    public class BaseConfigurationViewModel<T> : IWorkspaceConfigurationViewModel where T : IPropertyViewModel, IInstrumentationProperty, new()
    {
        /// <summary>
        /// Name of this configuration
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Description of this configuration
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Feature name of this configuration
        /// </summary>
        public string FeatureName { get; set; }

        /// <summary>
        /// Configuration flags
        /// </summary>
        public WorkspaceConfigurationFlag Flags { get; set; } = WorkspaceConfigurationFlag.None;

        /// <summary>
        /// Get the description for a message
        /// </summary>
        public string GetDescription(IWorkspaceViewModel workspaceViewModel)
        {
            return Description;
        }

        /// <summary>
        /// Install this configuration
        /// </summary>
        public void Install(IWorkspaceViewModel workspaceViewModel)
        {
            // Get instrumentable
            IPropertyViewModel? instrumentable = (workspaceViewModel.PropertyCollection as IInstrumentableObject)?.GetOrCreateInstrumentationProperty();
            if (instrumentable == null)
            {
                return;
            }
            
            // Get feature info
            FeatureInfo? featureInfo = workspaceViewModel.PropertyCollection.GetProperty<IFeatureCollectionViewModel>()?.GetFeature(FeatureName);
            if (featureInfo == null)
            {
                return;
            }
            
            // Add property
            instrumentable?.Properties.Add(new T()
            {
                Parent = workspaceViewModel.PropertyCollection,
                ConnectionViewModel = workspaceViewModel.Connection,
                FeatureInfo = featureInfo.Value
            });
        }
    }
}