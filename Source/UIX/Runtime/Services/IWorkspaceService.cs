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

using System;
using System.Reactive;
using System.Reactive.Subjects;
using DynamicData;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.Services
{
    public interface IWorkspaceService : IReactiveObject
    {
        /// <summary>
        /// All active workspaces
        /// </summary>
        IObservableList<ViewModels.Workspace.IWorkspaceViewModel> Workspaces { get; }
        
        /// <summary>
        /// All extensions
        /// </summary>
        public ISourceList<IWorkspaceExtension> Extensions { get; }
        
        /// <summary>
        /// All configurations
        /// </summary>
        public ISourceList<IWorkspaceConfigurationViewModel> Configurations { get; }
        
        /// <summary>
        /// Current workspace
        /// </summary>
        public ViewModels.Workspace.IWorkspaceViewModel? SelectedWorkspace { get; set; }
        
        /// <summary>
        /// Current selected property
        /// </summary>
        public IPropertyViewModel? SelectedProperty { get; set; }
        
        /// <summary>
        /// Current selected property
        /// </summary>
        public ShaderNavigationViewModel? SelectedShader { get; set; }
        
        /// <summary>
        /// Focus notification event
        /// </summary>
        public Subject<Unit> WorkspaceFocusNotify { get; }

        /// <summary>
        /// Add a new workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        void Add(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel);

        /// <summary>
        /// Install a given workspace
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        void Install(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel);

        /// <summary>
        /// Remove an existing workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        /// <returns>true if successfully removed</returns>
        bool Remove(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel);

        /// <summary>
        /// Clear all workspaces
        /// </summary>
        void Clear();
    }

    /// <summary>
    /// Extensions
    /// </summary>
    public static class WorkspaceServiceExtensions
    {
        /// <summary>
        /// Get a typed configuration of this service
        /// </summary>
        public static T? GetConfiguration<T>(this IWorkspaceService self) where T : class
        {
            foreach (IWorkspaceConfigurationViewModel workspaceConfigurationViewModel in self.Configurations.Items)
            {
                if (workspaceConfigurationViewModel is T typed)
                {
                    return typed;
                }
            }

            return null;
        }
    }
}