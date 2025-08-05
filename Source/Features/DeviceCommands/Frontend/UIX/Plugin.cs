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

using DynamicData;
using GRS.Features.DeviceCommands.UIX.Workspace;
using GRS.Features.DeviceCommands.UIX.Workspace.Properties.Instrumentation;
using Studio.Plugin;
using Studio.Services;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Configurations;
using UIX.Resources;

namespace GRS.Features.DeviceCommands.UIX
{
    public class Plugin : IPlugin, IWorkspaceExtension
    {
        public PluginInfo Info { get; } = new()
        {
            Name = "DeviceCommands",
            Description = "Validation of indirect device commands against user configured limits",
            Dependencies = new string[]{ }
        };
        
        /// <summary>
        /// Install this plugin
        /// </summary>
        /// <returns></returns>
        public bool Install()
        {
            // Get workspace service
            var workspaceService = ServiceRegistry.Get<IWorkspaceService>();
            
            // Add workspace extension
            workspaceService?.Extensions.Add(this);

            // OK
            return true;
        }

        /// <summary>
        /// Uninstall this plugin
        /// </summary>
        public void Uninstall()
        {
            // Remove workspace extension
            ServiceRegistry.Get<IWorkspaceService>()?.Extensions.Remove(this);
        }

        /// <summary>
        /// Install an extension
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        public void Install(IWorkspaceViewModel workspaceViewModel)
        {
            // Create service
            workspaceViewModel.PropertyCollection.Services.Add(new DeviceCommandsService(workspaceViewModel));
        }
    }
}
