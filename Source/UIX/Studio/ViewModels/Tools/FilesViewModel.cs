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
using System.Reactive.Disposables;
using System.Reactive.Linq;
using System.Windows.Input;
using Avalonia.Media;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Runtime.ViewModels.Tools;
using Studio.Services;
using Studio.Services.Suspension;
using Studio.ViewModels.Code;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Services;

namespace Studio.ViewModels.Tools
{
    public class FilesViewModel : ToolViewModel
    {
        /// <summary>
        /// Tooling icon
        /// </summary>
        public override StreamGeometry? Icon => ResourceLocator.GetIcon("ToolFiles");

        /// <summary>
        /// Tooling tip
        /// </summary>
        public override string? ToolTip => Resources.Resources.Tool_Files;
        
        /// <summary>
        /// Is the help message visible?
        /// </summary>
        public bool IsHelpVisible
        {
            get => _isHelpVisible;
            set => this.RaiseAndSetIfChanged(ref _isHelpVisible, value);
        }

        /// <summary>
        /// Help message
        /// </summary>
        public string HelpMessage
        {
            get => _helpMessage;
            set => this.RaiseAndSetIfChanged(ref _helpMessage, value);
        }

        /// <summary>
        /// Current filter string
        /// </summary>
        [DataMember]
        public string FilterString
        {
            get => _filterString;
            set => this.RaiseAndSetIfChanged(ref _filterString, value);
        }

        /// <summary>
        /// All child items
        /// </summary>
        public ObservableCollectionExtended<IObservableTreeItem> Files { get; } = new();

        /// <summary>
        /// All filtered child items
        /// </summary>
        public ObservableCollectionExtended<IObservableTreeItem> FilteredFiles
        {
            get => _filteredFiles;
            set => this.RaiseAndSetIfChanged(ref _filteredFiles, value);
        }

        /// <summary>
        /// Connect to new workspace
        /// </summary>
        public ICommand Expand { get; }
        
        /// <summary>
        /// Connect to new workspace
        /// </summary>
        public ICommand Collapse { get; }

        /// <summary>
        /// View model associated with this property
        /// </summary>
        public ShaderNavigationViewModel? ShaderViewModel
        {
            get => _shaderViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _shaderViewModel, value);
                OnShaderChanged();
            }
        }

        public FilesViewModel()
        {
            Expand = ReactiveCommand.Create(OnExpand);
            Collapse = ReactiveCommand.Create(OnCollapse);

            // Must have service
            if (ServiceRegistry.Get<IWorkspaceService>() is not { } workspaceService)
            {
                return;
            }
            
            // Notify on query string change
            this.WhenAnyValue(x => x.FilterString)
                .Throttle(TimeSpan.FromMilliseconds(250))
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(x => FilterFiles());

            // Bind selected workspace
            workspaceService
                .WhenAnyValue(x => x.SelectedWorkspace)
                .WhereNotNull()
                .Subscribe(OnWorkspaceIndexedPooling);
            
            // Bind selected shader
            workspaceService
                .WhenAnyValue(x => x.SelectedShader)
                .Subscribe(x =>
                {
                    ShaderViewModel = x;
                    Owner?.Factory?.SetActiveDockable(this);
                });
            
            // Empty filter
            FilterFiles();
            
            // Suspension
            this.BindTypedSuspension();
        }

        /// <summary>
        /// Run the filter
        /// </summary>
        private void FilterFiles()
        {
            if (string.IsNullOrEmpty(_filterString))
            {
                FilteredFiles = Files;
                return;
            }
            
            // Filter outside the main collection, avoids needless observer events
            List<IObservableTreeItem> files = new();
            
            // Filter from roots, do not include roots
            foreach (IObservableTreeItem observableTreeItem in Files)
            {
                FilterFlat(observableTreeItem, files);
            }
            
            // Assign new files
            using (Files.SuspendNotifications())
            {
                FilteredFiles = new ObservableCollectionExtended<IObservableTreeItem>(files);
            }
        }
        
        /// <summary>
        /// Filter a file hierarchy
        /// </summary>
        private void FilterFlat(IObservableTreeItem item, List<IObservableTreeItem> files)
        {
            // Contains string?
            if (item.Text.Contains(_filterString, StringComparison.InvariantCultureIgnoreCase))
            {
                files.Add(new FileTreeItemViewModel()
                {
                    Text = item.Text,
                    StatusColor = item.StatusColor,
                    ViewModel = item.ViewModel
                });
            }

            // Filter all children
            foreach (IObservableTreeItem child in item.Items)
            {
                FilterFlat(child, files);
            }
        }

        /// <summary>
        /// Invoked on expansion
        /// </summary>
        private void OnExpand()
        {
            foreach (IObservableTreeItem file in Files)
            {
                ExpandFile(file, true);
            }
        }

        /// <summary>
        /// Invoked on collapses
        /// </summary>
        private void OnCollapse()
        {
            foreach (IObservableTreeItem file in Files)
            {
                ExpandFile(file, false);
            }
        }

        /// <summary>
        /// Set the expansion state of an item
        /// </summary>
        private void ExpandFile(IObservableTreeItem file, bool expanded)
        {
            file.IsExpanded = expanded;

            // Handle children
            foreach (IObservableTreeItem item in file.Items)
            {
                ExpandFile(item, expanded);
            }
        }

        /// <summary>
        /// Invoked on shader changes
        /// </summary>
        private void OnShaderChanged()
        {
            _lastFileComposite.Clear();

            // Subscribe to pooling
            ShaderViewModel?.Shader?.WhenAnyValue(x => x.FileViewModels)
                .SelectMany(items => items.ToObservableChangeSet())
                .Subscribe(_ => OnShaderFilePooling())
                .DisposeWith(_lastFileComposite);

            // Initial pooling
            OnShaderFilePooling();
        }

        /// <summary>
        /// Invoked on workspace pooling
        /// </summary>
        private void OnWorkspaceIndexedPooling(IWorkspaceViewModel workspaceViewModel)
        {
            if (workspaceViewModel.PropertyCollection.GetService<IFileCodeService>() is { } fileCodeService)
            {
                // Remove last subscription
                _workspaceComposite.Clear();

                // Bind to indexing events
                fileCodeService
                    .WhenAnyValue(x => x.Indexed)
                    .Subscribe(_ => OnWorkspaceIndexed(fileCodeService))
                    .DisposeWith(_workspaceComposite);
            }
        }

        /// <summary>
        /// Remove a given root item and reset
        /// </summary>
        private void RemoveRootAndInvalidate(ref IObservableTreeItem? item)
        {
            if (item != null)
            {
                Files.Remove(item);
                item = null;
            }
        }

        /// <summary>
        /// Invoked on workspace index completions
        /// </summary>
        private void OnWorkspaceIndexed(IFileCodeService fileCodeService)
        {
            // Filter outside the main collection, avoids needless observer events
            List<IObservableTreeItem> files = new();

            // No files at all?
            if (fileCodeService.Files.Count == 0)
            {
                RemoveRootAndInvalidate(ref _workspaceIndexFileRoot);
                FilterFiles();
                return;
            }

            // Root node
            FileNode root = new();
            
            // Populate all files
            foreach (CodeFileViewModel codeFileViewModel in fileCodeService.Files)
            {
                InsertNode(root, codeFileViewModel);
            }
            
            // Collapse root
            CollapseNode(root, files, _fileWorkspaceColor, false);

            // Rename root
            IObservableTreeItem rootItem = files[0];
            rootItem.Text = "Workspace";
            rootItem.IsExpanded = true;

            // Replace file collection
            using (Files.SuspendNotifications())
            {
                RemoveRootAndInvalidate(ref _workspaceIndexFileRoot);
                Files.Add(rootItem);
                _workspaceIndexFileRoot = rootItem;
            }
            
            // Run the filter again
            FilterFiles();
        }

        /// <summary>
        /// Pull in latest file changes
        /// </summary>
        private void OnShaderFilePooling()
        {
            // Filter outside the main collection, avoids needless observer events
            List<IObservableTreeItem> files = new();

            // TODO: Feed message through localization
            
            // Invalid?
            if (ShaderViewModel?.Shader == null)
            {
                IsHelpVisible = true;
                HelpMessage = "No shader selected";
                RemoveRootAndInvalidate(ref _selectedShaderFileRoot);
                FilterFiles();
                return;
            }

            // No files at all?
            if (ShaderViewModel.Shader.FileViewModels.Count == 0)
            {
                IsHelpVisible = true;
                HelpMessage = "No files";
                RemoveRootAndInvalidate(ref _selectedShaderFileRoot);
                FilterFiles();
                return;
            }

            // Root node
            FileNode root = new();
            
            // Populate node tree
            foreach (ShaderFileViewModel shaderFileViewModel in ShaderViewModel.Shader.FileViewModels)
            {
                // Skip effectively empty files
                if (string.IsNullOrWhiteSpace(shaderFileViewModel.Contents))
                {
                    continue;
                }
                
                InsertNode(root, shaderFileViewModel);
            }
            
            // Collapse root
            CollapseNode(root, files, Brushes.White, true);

            // Rename root
            IObservableTreeItem rootItem = files[0];
            rootItem.Text = "Selected Shader";
            rootItem.IsExpanded = true;
            
            // Replace file collection
            using (Files.SuspendNotifications())
            {
                RemoveRootAndInvalidate(ref _selectedShaderFileRoot);
                Files.Insert(0, rootItem);
                _selectedShaderFileRoot =  rootItem;
            }
            
            // Run the filter again
            FilterFiles();
            
            // Events
            this.RaisePropertyChanged(nameof(IsHelpVisible));

            // No collapsed children?
            if (root.Children.Count == 0)
            {
                IsHelpVisible = true;
                HelpMessage = "All files empty";
                return;
            }
            
            // Valid files
            IsHelpVisible = false;
        }

        /// <summary>
        /// Collapse a given node
        /// </summary>
        private void CollapseNode(FileNode node, IList<IObservableTreeItem> collection, IBrush? statusColor, bool expanded)
        {
            // Leaf node?
            if (node.File != null)
            {
                collection.Add(new FileTreeItemViewModel()
                {
                    StatusColor = statusColor,
                    Text = node.Name,
                    ViewModel = node.File
                });
                
                return;
            }

            // Effective split point?
            if (node.IsSplitPoint)
            {
                string filename = node.Name;

                // Merge directories up until a split point
                FileNode? parent = node.Parent;
                while (parent != null && !parent.IsSplitPoint)
                {
                    filename = System.IO.Path.Join(parent.Name, filename);
                    parent = parent.Parent;
                }
                
                // Create item
                var treeItem = new FileTreeItemViewModel()
                {
                    IsExpanded = expanded,
                    StatusColor = statusColor,
                    Text = filename == string.Empty ? "//" : filename,
                    ViewModel = node.File
                };

                // First file index
                int firstFileIndex = 0;
                for (; firstFileIndex < collection.Count; firstFileIndex++)
                {
                    if (collection[firstFileIndex].ViewModel != null)
                    {
                        break;
                    }
                }

                // OK
                collection.Insert(firstFileIndex, treeItem);
                collection = treeItem.Items;
            }
            
            // Collapse all children
            foreach (var child in node.Children)
            {
                CollapseNode(child, collection, statusColor, expanded);
            }
        }

        /// <summary>
        /// Insert a node
        /// </summary>
        /// <param name="node">search node</param>
        /// <param name="file">file to insert</param>
        private void InsertNode(FileNode node, CodeFileViewModel file)
        {
            // If there's a directory name, create a node tree
            string? immediateDirectory = System.IO.Path.GetDirectoryName(file.Filename);
            if (immediateDirectory != null)
            {
                // Directory list
                List<string> directories = new();
                directories.Add(immediateDirectory);

                // Reduce directories
                while (System.IO.Path.GetDirectoryName(directories.Last()) is { } next)
                {
                    directories.Add(next);
                }

                // Visit in reverse order
                directories.Reverse();
            
                // Create tree
                foreach (string directory in directories)
                {
                    // Get base name of the directory
                    string basename = System.IO.Path.GetFileName(directory);
                    if (basename.Length == 0)
                    {
                        basename = directory;
                    }
                    
                    // Search for existing node
                    if (node.Children.Any(x =>
                        {
                            if (x.Name != basename)
                            {
                                return false;
                            }

                            node = x;
                            return true;
                        }))
                    {
                        continue;
                    }

                    // No node found, create a new one
                    var directoryNode = new FileNode()
                    {
                        Name = basename,
                        Parent = node
                    };
                
                    // Add and search from this node
                    node.Children.Add(directoryNode);

                    // Split point if there's more than a single child
                    if (node.Children.Count > 1)
                    {
                        node.IsSplitPoint = true;
                    }
                    
                    node = directoryNode;
                }
            }

            // Leaf nodes always mean split points
            node.IsSplitPoint = true;

            // Add file leaf
            node.Children.Add(new FileNode()
            {
                Name = System.IO.Path.GetFileName(file.Filename),
                Parent = node,
                File = file
            });
        }

        private class FileNode
        {
            /// <summary>
            /// Name of this node
            /// </summary>
            public string Name;

            /// <summary>
            /// Is this node a split point?
            /// </summary>
            public bool IsSplitPoint = false;

            /// <summary>
            /// Optional, leaf view model
            /// </summary>
            public CodeFileViewModel? File;

            /// <summary>
            /// Parent of this node
            /// </summary>
            public FileNode? Parent;
            
            /// <summary>
            /// All children of this node
            /// </summary>
            public List<FileNode> Children = new();
        }
        
        /// <summary>
        /// Internal view model
        /// </summary>
        private ShaderNavigationViewModel? _shaderViewModel;

        /// <summary>
        /// Disposers
        /// </summary>
        private CompositeDisposable _lastFileComposite = new();
        private CompositeDisposable _workspaceComposite = new();

        /// <summary>
        /// All root items
        /// </summary>
        private IObservableTreeItem? _selectedShaderFileRoot;
        private IObservableTreeItem? _workspaceIndexFileRoot;

        /// <summary>
        /// Internal, default, connection string
        /// </summary>
        private string _filterString = "";

        /// <summary>
        /// Internal help state
        /// </summary>
        private bool _isHelpVisible = true;

        /// <summary>
        /// Internal message state
        /// </summary>
        private string _helpMessage = "No files";

        /// <summary>
        /// All brushes
        /// </summary>
        private IBrush? _fileWorkspaceColor = ResourceLocator.GetBrush("FileWorkspaceColor");

        /// <summary>
        /// Internal filtered reference
        /// </summary>
        private ObservableCollectionExtended<IObservableTreeItem> _filteredFiles;
    }
}