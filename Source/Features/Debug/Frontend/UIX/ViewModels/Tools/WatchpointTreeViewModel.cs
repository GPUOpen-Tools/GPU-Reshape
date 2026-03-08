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
using System.IO;
using System.Linq;
using System.Reactive.Disposables;
using System.Windows.Input;
using Avalonia.Media;
using DynamicData;
using DynamicData.Binding;
using GRS.Features.Debug.UIX.ViewModels.Controls;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Runtime.ViewModels.Tools;
using Studio;
using Studio.Models.Workspace.Objects;
using Studio.Services;
using Studio.ViewModels.Code;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.Debug.UIX.ViewModels.Tools
{
    public class WatchpointTreeViewModel : ToolViewModel
    {
        /// <summary>
        /// Tooling icon
        /// </summary>
        public override StreamGeometry? Icon => ResourceLocator.GetIcon("ToolWatchpointTree");

        public WatchpointTreeItemViewModel Root { get; } = new()
        {
            IsExpanded = true
        };

        /// <summary>
        /// Tooling tip
        /// </summary>
        public override string? ToolTip => "All shader watchpoints";
        
        /// <summary>
        /// Is the help message visible?
        /// </summary>
        public bool IsHelpVisible => Root.Items.Count == 0;

        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IWorkspaceViewModel? WorkspaceViewModel
        {
            get => _workspaceViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _workspaceViewModel, value);
                OnConnectionChanged();
            }
        }

        /// <summary>
        /// Opens a given watchpoint document from view model
        /// </summary>
        public ICommand OpenWatchpointDocument;

        public WatchpointTreeViewModel()
        {
            Title = "Watchpoints";
            
            OpenWatchpointDocument = ReactiveCommand.Create<WatchpointTreeItemViewModel>(OnOpenWatchpointDocument);
            
            // Bind selected workspace
            ServiceRegistry.Get<IWorkspaceService>()?
                .WhenAnyValue(x => x.SelectedWorkspace)
                .Subscribe(x => WorkspaceViewModel = x);
        }

        /// <summary>
        /// Update help visibility
        /// </summary>
        private void UpdateHelp()
        {
            this.RaisePropertyChanged(nameof(IsHelpVisible));
        }

        /// <summary>
        /// Invoked on document handlers
        /// </summary>
        private void OnOpenWatchpointDocument(WatchpointTreeItemViewModel watchpointIdentifierViewModel)
        {
            if (_workspaceViewModel == null)
                return;
            
            if (ServiceRegistry.Get<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                switch (watchpointIdentifierViewModel.ViewModel)
                {
                    case ShaderFileViewModel shaderFileViewModel:
                    {
                        layoutViewModel.DocumentLayout?.OpenDocument(new ShaderDescriptor()
                        {
                            PropertyCollection = _workspaceViewModel?.PropertyCollection,
                            GUID = shaderFileViewModel.ShaderGUID,
                            StartupLocation = new NavigationLocation()
                            {
                                Location = new ShaderLocation()
                                {
                                    FileUID = (int)shaderFileViewModel.UID
                                }
                            }
                        });
                        break;
                    }
                    case ShaderViewModel shaderViewModel:
                    {
                        layoutViewModel.DocumentLayout?.OpenDocument(new ShaderDescriptor()
                        {
                            PropertyCollection = _workspaceViewModel?.PropertyCollection,
                            GUID = shaderViewModel.GUID
                        });
                        break;
                    }
                    case CodeFileViewModel fileViewModel:
                    {
                        layoutViewModel.DocumentLayout?.OpenDocument(new CodeFileDescriptor()
                        {
                            PropertyCollection = _workspaceViewModel?.PropertyCollection,
                            CodeFileViewModel = fileViewModel
                        });
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            // Clear states
            _disposable.Clear();
            Root.Items.Clear();
            UpdateHelp();

            // Must be valid workspace
            if (_workspaceViewModel is not { Connection: { } })
                return;

            // Must have collection registry
            if (_workspaceViewModel.PropertyCollection.GetProperty<WatchpointCollectionRegistryViewModel>() is not
                { } collectionRegistryViewModel)
                return;

            // Subscribe to collections
            collectionRegistryViewModel.Collections
                .ToObservableChangeSet()
                .OnItemAdded(OnCollectionAdded)
                .OnItemRemoved(OnCollectionRemoved)
                .Subscribe()
                .DisposeWith(_disposable);
        }

        /// <summary>
        /// Invoked on collection additions
        /// </summary>
        private void OnCollectionAdded(KeyValuePair<object, WatchpointCollectionViewModel> obj)
        {
            // Create item
            var collectionItem = new WatchpointTreeItemViewModel()
            {
                Text = GetDisplayName(obj.Key),
                ViewModel = obj.Key,
                LookupKey = obj.Key,
                IsExpanded = true
            };
            
            // Subscribe to all bindings
            obj.Value.SourceBindings
                .ToObservableChangeSet()
                .OnItemAdded(x => OnBindingAdded(collectionItem, x))
                .OnItemRemoved(x => OnBindingRemoved(collectionItem, x))
                .Subscribe()
                .DisposeWith(_disposable);
            
            Root.Items.Add(collectionItem);
            UpdateHelp();
        }

        /// <summary>
        /// Invoked on collection removals
        /// </summary>
        private void OnCollectionRemoved(KeyValuePair<object, WatchpointCollectionViewModel> obj)
        {
            // Remove matching view model
            if (Root.Items.FirstOrDefault(x => ((WatchpointTreeItemViewModel)x).LookupKey == obj.Key) is {} child)
            {
                Root.Items.Remove(child);
            }
            
            UpdateHelp();
        }

        /// <summary>
        /// Invoked on binding additions
        /// </summary>
        private void OnBindingAdded(WatchpointTreeItemViewModel item, WatchpointViewModelSourceBinding binding)
        {
            // Create item
            var watchpointItem = new WatchpointTreeItemViewModel()
            {
                ViewModel = item.ViewModel,
                LookupKey = binding.WatchpointViewModel,
                Text = $"Watchpoint {binding.WatchpointViewModel.UID}",
                IsExpanded = true
            };
            
            // Add the chosen source mapping
            watchpointItem.Items.Add(new WatchpointTreeItemViewModel()
            {
                ViewModel = item.ViewModel,
                LookupKey = binding.WatchpointViewModel,
                Text = $"Instruction - BB: {binding.Source.Mapping.BasicBlockId}, I: {binding.Source.Mapping.InstructionIndex}",
                IsExpanded = true
            });

            item.Items.Add(watchpointItem);
        }

        /// <summary>
        /// Invoked on binding removals
        /// </summary>
        private void OnBindingRemoved(WatchpointTreeItemViewModel item, WatchpointViewModelSourceBinding binding)
        {
            if (item.Items.FirstOrDefault(x => ((WatchpointTreeItemViewModel)x).LookupKey == binding.WatchpointViewModel) is {} child)
            {
                item.Items.Remove(child);
            }
        }

        /// <summary>
        /// Get the display name of a view model
        /// </summary>
        private string GetDisplayName(object obj)
        {
            switch (obj)
            {
                default:
                    return "Unknown";
                case IPropertyViewModel property:
                    return property.Name;
                case ShaderFileViewModel shaderFileViewModel:
                    return $"Shader File {shaderFileViewModel.UID} - {Path.GetFileName(shaderFileViewModel.Filename)}";
                case ShaderViewModel shaderViewModel:
                    return $"Shader {shaderViewModel.GUID} - {Path.GetFileName(shaderViewModel.Filename)}";
                case CodeFileViewModel fileViewModel:
                    return Path.GetFileName(fileViewModel.Filename);
            }
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private IWorkspaceViewModel? _workspaceViewModel;

        /// <summary>
        /// Shared event disposable
        /// </summary>
        private CompositeDisposable _disposable = new();
    }
}