// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
using System.Collections.ObjectModel;
using System.Reactive;
using Avalonia;
using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.Services;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Documents
{
    public class WorkspaceOverviewViewModel : Document, IDocumentViewModel
    {
        /// <summary>
        /// Descriptor setter, constructs the top document from given descriptor
        /// </summary>
        public IDescriptor? Descriptor
        {
            get => _descriptor;
            set
            {
                // Valid descriptor?
                if (value is not WorkspaceOverviewDescriptor { } descriptor)
                {
                    return;
                }

                // Construction descriptor?
                if (_descriptor == null)
                {
                    ConstructDescriptor(descriptor);
                }
                
                _descriptor = descriptor;
            }
        }

        /// <summary>
        /// Construct from a given descriptor
        /// </summary>
        /// <param name="descriptor"></param>
        private void ConstructDescriptor(WorkspaceOverviewDescriptor descriptor)
        {
            Id = "Workspace";
            Title = $"{descriptor.Workspace?.Connection?.Application?.DecoratedName ?? "Invalid"}";
            Workspace = descriptor.Workspace;
        }

        /// <summary>
        /// Document icon
        /// </summary>
        public StreamGeometry? Icon
        {
            get => _icon;
            set => this.RaiseAndSetIfChanged(ref _icon, value);
        }

        /// <summary>
        /// Icon color
        /// </summary>
        public Color? IconForeground
        {
            get => _iconForeground;
            set => this.RaiseAndSetIfChanged(ref _iconForeground, value);
        }

        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public IWorkspaceViewModel? Workspace
        {
            get => _workspaceViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _workspaceViewModel, value);
                OnWorkspaceChanged();
            }
        }

        /// <summary>
        /// All properties of this view
        /// </summary>
        public ObservableCollection<IPropertyViewModel> Properties { get; } = new();

        /// <summary>
        /// Invoked on selection
        /// </summary>
        public override void OnSelected()
        {
            base.OnSelected();
            
            // Set selected workspace
            if (App.Locator.GetService<IWorkspaceService>() is { } service)
            {
                // Update workspace
                service.SelectedWorkspace = Workspace;
                
                // Force focus
                service.WorkspaceFocusNotify.OnNext(Unit.Default);
            }
        }
        
        /// <summary>
        /// Invoked when the workspace has changed
        /// </summary>
        public void OnWorkspaceChanged()
        {
            // Filter the owning collection properties
            _workspaceViewModel?.PropertyCollection.Properties.Connect()
                .OnItemAdded(x =>
                {
                    if (x.Visibility.HasFlag(PropertyVisibility.WorkspaceOverview))
                    {
                        Properties.Add(x);
                    }
                })
                .OnItemRemoved(x =>
                {
                    Properties.Remove(x);
                })
                .Subscribe();
        }

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IWorkspaceViewModel? _workspaceViewModel;
        
        /// <summary>
        /// Internal icon
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("StreamOn");

        /// <summary>
        /// Internal descriptor
        /// </summary>
        private WorkspaceOverviewDescriptor? _descriptor;

        /// <summary>
        /// Internal icon color
        /// </summary>
        private Color? _iconForeground = ResourceLocator.GetResource<Color>("ThemeForegroundColor");
    }
}