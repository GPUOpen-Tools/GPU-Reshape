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

using System.Linq;
using System.Text;
using DynamicData;
using Studio.Extensions;

namespace Studio.ViewModels.Workspace.Configurations
{
    public class BasicConfigurationViewModel : IBasicConfigurationViewModel
    {
        /// <summary>
        /// Name of this configuration
        /// </summary>
        public string Name { get; } = Resources.Resources.Workspace_Configuration_Basic_Name;

        /// <summary>
        /// Configuration flags
        /// </summary>
        public WorkspaceConfigurationFlag Flags => WorkspaceConfigurationFlag.CanDetail;

        /// <summary>
        /// All configurations within
        /// </summary>
        public ISourceList<IWorkspaceConfigurationViewModel> Configurations { get; } = new SourceList<IWorkspaceConfigurationViewModel>();

        /// <summary>
        /// Get the description for a message
        /// </summary>
        public string GetDescription(IWorkspaceViewModel workspaceViewModel)
        {
            return $"{Resources.Resources.Workspace_Configuration_Basic_Header} {Configurations.Items.NaturalJoin(x => x.Name)}";
        }

        /// <summary>
        /// Install this configuration
        /// </summary>
        public void Install(IWorkspaceViewModel workspaceViewModel)
        {
            // Install all workspaces
            foreach (IWorkspaceConfigurationViewModel workspaceConfigurationViewModel in Configurations.Items)
            {
                workspaceConfigurationViewModel.Install(workspaceViewModel);
            }
        }
    }
}