﻿// 
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
using Avalonia;
using DynamicData;
using GRS.Features.ExportIndices.UIX.Workspace;
using GRS.Features.ExportIndices.UIX.Workspace.Properties.Instrumentation;
using Studio.Plugin;
using Studio.Services;
using Studio.ViewModels.Contexts;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Configurations;
using UIX.Resources;

namespace GRS.Features.ExportIndices.UIX
{
    public class Plugin : IPlugin, IWorkspaceExtension
    {
        /// <summary>
        /// Plugin info
        /// </summary>
        public PluginInfo Info { get; } = new()
        {
            Name = "ExportIndices",
            Description = "Provides instrumentation around export indices for vertices and primitives in mesh shader",
            Dependencies = new string[]{ }
        };
        
        /// <summary>
        /// Install this plugin
        /// </summary>
        /// <returns></returns>
        public bool Install()
        {
            // Get workspace service
            var workspaceService = AvaloniaLocator.Current.GetService<IWorkspaceService>();
            
            // Add workspace extension
            workspaceService?.Extensions.Add(this);
            
            // Ignore the All configuration
            workspaceService?.GetConfiguration<IAllConfigurationViewModel>()?.IgnoredFeatures.Add("Export Indices");
            
            // Add workspace configuration
            workspaceService?.GetConfiguration<IBasicConfigurationViewModel>()?.Configurations.Add(new BaseConfigurationViewModel<ExportIndicesPropertyViewModel>()
            {
                Name = Resources.Workspace_Configuration_ExportIndices_Name,
                Description = Resources.Workspace_Configuration_ExportIndices_Description,
                Flags = WorkspaceConfigurationFlag.CanDetail,
                FeatureName = "Export Indices"
            });

            // OK
            return true;
        }

        /// <summary>
        /// Uninstall this plugin
        /// </summary>
        public void Uninstall()
        {
            // Remove workspace extension
            AvaloniaLocator.Current.GetService<IWorkspaceService>()?.Extensions.Remove(this);
        }

        /// <summary>
        /// Install an extension
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        public void Install(IWorkspaceViewModel workspaceViewModel)
        {
            // Create service
            workspaceViewModel.PropertyCollection.Services.Add(new ExportIndicesService(workspaceViewModel));
        }
    }
}
