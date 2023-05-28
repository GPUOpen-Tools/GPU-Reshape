using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive.Disposables;
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
        public bool IsHelpVisible => (ShaderViewModel?.Shader?.FileViewModels.Count ?? 0) == 0;
        
        /// <summary>
        /// All child items
        /// </summary>
        public ObservableCollection<IObservableTreeItem> Files { get; } = new();
        
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
            App.Locator.GetService<IWorkspaceService>()?
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

            // Subscribe to future pools
            ShaderViewModel?.Shader?.FileViewModels.ToObservableChangeSet()
                .OnItemAdded(_ => OnShaderFilePooling())
                .Subscribe()
                .DisposeWith(_lastFileComposite);
            
            // Initial pooling
            OnShaderFilePooling();
        }

        /// <summary>
        /// Pull in latest file changes
        /// </summary>
        private void OnShaderFilePooling()
        {
            Files.Clear();

            // Invalid?
            if (ShaderViewModel?.Shader == null)
            {
                return;
            }

            // Root node
            FileNode root = new();
            
            // Populate node tree
            foreach (ShaderFileViewModel shaderFileViewModel in ShaderViewModel.Shader.FileViewModels)
            {
                InsertNode(root, shaderFileViewModel);
            }
            
            // Collapse all children, root is detached in nature
            foreach (FileNode child in root.Children)
            {
                CollapseNode(child, Files);
            }

            // Events
            this.RaisePropertyChanged(nameof(IsHelpVisible));
        }

        /// <summary>
        /// Collapse a given node
        /// </summary>
        /// <param name="node">target node</param>
        /// <param name="collection">collection to append to</param>
        private void CollapseNode(FileNode node, ObservableCollection<IObservableTreeItem> collection)
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
                    Text = filename,
                    ViewModel = node.File
                };

                // OK
                collection.Add(treeItem);
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
    }
}