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

using System.Reactive;
using System.Reactive.Subjects;
using Avalonia;
using DynamicData;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.Extensions;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.Services
{
    public class WorkspaceService : ReactiveObject, IWorkspaceService
    {
        /// <summary>
        /// Active workspaces
        /// </summary>
        public IObservableList<ViewModels.Workspace.IWorkspaceViewModel> Workspaces => _workspaces;

        /// <summary>
        /// All extensions
        /// </summary>
        public ISourceList<IWorkspaceExtension> Extensions { get; } = new SourceList<IWorkspaceExtension>();

        /// <summary>
        /// Current workspace
        /// </summary>
        public IWorkspaceViewModel? SelectedWorkspace
        {
            get => _selectedWorkspace;
            set => this.RaiseAndSetIfChanged(ref _selectedWorkspace, value);
        }

        /// <summary>
        /// Current selected property
        /// </summary>
        public IPropertyViewModel? SelectedProperty
        {
            get => _selectedProperty;
            set => this.RaiseAndSetIfChanged(ref _selectedProperty, value);
        }

        /// <summary>
        /// Current selected property
        /// </summary>
        public ShaderNavigationViewModel? SelectedShader
        {
            get => _selectedShader;
            set => this.RaiseAndSetIfChanged(ref _selectedShader, value);
        }

        /// <summary>
        /// Focus notification event
        /// </summary>
        public Subject<Unit> WorkspaceFocusNotify { get; } = new();

        public WorkspaceService()
        {
            
        }

        /// <summary>
        /// Add a new workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        public void Add(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel)
        {
            _workspaces.Add(workspaceViewModel);
            
            // Submit document
            if (App.Locator.GetService<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.DocumentLayout?.OpenDocument(new WorkspaceOverviewDescriptor()
                {
                    Workspace = workspaceViewModel 
                });
            }
            
            // Diagnostic
            Logging.Info($"Workspace created for {workspaceViewModel.Connection?.Application?.Name} {{{workspaceViewModel.Connection?.Application?.Guid}}}");
            
            // Assign as selected
            SelectedWorkspace = workspaceViewModel;
        }

        /// <summary>
        /// Install a workspace
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        public void Install(IWorkspaceViewModel workspaceViewModel)
        {
            foreach (IWorkspaceExtension workspaceExtension in Extensions.Items)
            {
                workspaceExtension.Install(workspaceViewModel);
            }
        }

        /// <summary>
        /// Remove an existing workspace
        /// </summary>
        /// <param name="workspaceViewModel">active connection</param>
        /// <returns>true if successfully removed</returns>
        public bool Remove(ViewModels.Workspace.IWorkspaceViewModel workspaceViewModel)
        {
            // Diagnostic
            Logging.Info($"Closed workspace for {workspaceViewModel.Connection?.Application?.Name}");
            
            // Is selected?
            if (SelectedWorkspace == workspaceViewModel)
            {
                SelectedWorkspace = null;
            }
            
            // Close all documents
            CloseDocumentsFor(workspaceViewModel);
           
            // Try to remove
            return _workspaces.Remove(workspaceViewModel);
        }

        /// <summary>
        /// Clear all workspaces
        /// </summary>
        public void Clear()
        {
            if (_workspaces.Count > 0)
            {
                // Diagnostic
                Logging.Info($"Closing all workspaces");

                // Remove selected
                SelectedWorkspace = null;
                
                // Close all documents
                _workspaces.Items.ForEach(x => CloseDocumentsFor(x));
            
                // Remove all
                _workspaces.Clear();
            }
        }

        /// <summary>
        /// Close all documents for a specific workspace
        /// </summary>
        private void CloseDocumentsFor(IWorkspaceViewModel owner)
        {
            if (App.Locator.GetService<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.DocumentLayout?.CloseOwnedDocuments(owner.PropertyCollection);
            }
        }

        /// <summary>
        /// Internal active workspaces
        /// </summary>
        private SourceList<ViewModels.Workspace.IWorkspaceViewModel> _workspaces = new();

        /// <summary>
        /// Internal active workspace
        /// </summary>
        private IWorkspaceViewModel? _selectedWorkspace;

        /// <summary>
        /// Internal selected property
        /// </summary>
        private IPropertyViewModel? _selectedProperty;

        /// <summary>
        /// Internal selected shader
        /// </summary>
        private ShaderNavigationViewModel? _selectedShader;
    }
}