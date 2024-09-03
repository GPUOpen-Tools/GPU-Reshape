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
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive.Disposables;
using System.Reactive.Linq;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Avalonia.Threading;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.ViewModels.Objects;
using Runtime.ViewModels.Shader;
using Runtime.ViewModels.Tools;
using Studio.Services;
using Studio.ViewModels.Controls;

namespace Studio.ViewModels.Tools
{
    public class FilesViewModel : ToolViewModel
    {
        /// <summary>
        /// Tooling icon
        /// </summary>
        public override StreamGeometry? Icon => ResourceLocator.GetIcon("ToolFiles");
        
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
        /// All child items
        /// </summary>
        public ObservableCollectionExtended<IObservableTreeItem> Files { get; } = new();
        
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
            
            // Bind selected workspace
            ServiceRegistry.Get<IWorkspaceService>()?
                .WhenAnyValue(x => x.SelectedShader)
                .Subscribe(x =>
                {
                    ShaderViewModel = x;
                    Owner?.Factory?.SetActiveDockable(this);
                });
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
                Files.Clear();
                return;
            }

            // No files at all?
            if (ShaderViewModel.Shader.FileViewModels.Count == 0)
            {
                IsHelpVisible = true;
                HelpMessage = "No files";
                Files.Clear();
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
            
            // Collapse all children, root is detached in nature
            foreach (FileNode child in root.Children)
            {
                CollapseNode(child, files);
            }

            // Replace file collection
            using (Files.SuspendNotifications())
            {
                Files.Clear();
                Files.AddRange(files);
            }
            
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
        /// <param name="node">target node</param>
        /// <param name="collection">collection to append to</param>
        private void CollapseNode(FileNode node, IList<IObservableTreeItem> collection)
        {
            // Leaf node?
            if (node.File != null)
            {
                collection.Add(new FileTreeItemViewModel()
                {
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
                CollapseNode(child, collection);
            }
        }

        /// <summary>
        /// Insert a node
        /// </summary>
        /// <param name="node">search node</param>
        /// <param name="file">file to insert</param>
        private void InsertNode(FileNode node, ShaderFileViewModel file)
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
                    node = directoryNode;

                    // Split point if there's more than a single child
                    if (node.Children.Count > 1)
                    {
                        node.IsSplitPoint = true;
                    }
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
            public ShaderFileViewModel? File;

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
        /// Disposer
        /// </summary>
        private CompositeDisposable _lastFileComposite = new();

        /// <summary>
        /// Internal help state
        /// </summary>
        private bool _isHelpVisible = true;

        /// <summary>
        /// Internal message state
        /// </summary>
        private string _helpMessage = "No files";
    }
}