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
using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia.Media;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.Utils.Workspace;
using Runtime.ViewModels.Shader;
using Runtime.ViewModels.Traits;
using Studio.Models.Workspace.Objects;
using Studio.Services;
using Studio.ViewModels.Code;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Properties;
using ShaderViewModel = Studio.ViewModels.Workspace.Objects.ShaderViewModel;

namespace Studio.ViewModels.Shader
{
    public class CodeContentViewModel : ReactiveObject, IShaderContentViewModel, ITextualContent
    {
        /// <summary>
        /// The owning navigation context
        /// </summary>
        public INavigationContext? NavigationContext { get; set; }

        /// <summary>
        /// Given descriptor
        /// </summary>
        public ShaderDescriptor? Descriptor
        {
            set
            {
                NavigationLocation = null;
                NavigationLocation = value?.StartupLocation;
            }
        }

        /// <summary>
        /// View icon
        /// </summary>
        public StreamGeometry? Icon
        {
            get => _icon;
            set => this.RaiseAndSetIfChanged(ref _icon, value);
        }

        /// <summary>
        /// Tooling tip
        /// </summary>
        public string? ToolTip => Resources.Resources.ShaderView_Code;
        
        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public IPropertyViewModel? PropertyCollection
        {
            get => _propertyCollection;
            set => this.RaiseAndSetIfChanged(ref _propertyCollection, value);
        }

        /// <summary>
        /// All services
        /// </summary>
        public ObservableCollection<IDestructableObject> Services { get; } = new();

        /// <summary>
        /// Currently selected validation object
        /// </summary>
        public ITextualSourceObject? SelectedTextualSourceObject
        {
            get => _selectedSourceObject;
            set => this.RaiseAndSetIfChanged(ref _selectedSourceObject, value);
        }

        /// <summary>
        /// Current detail view model
        /// </summary>
        public ISourceObjectDetailViewModel? DetailViewModel
        {
            get => _detailViewModel;
            set => this.RaiseAndSetIfChanged(ref _detailViewModel, value);
        }

        /// <summary>
        /// Selection command
        /// </summary>
        public ICommand? OnSelected { get; }

        /// <summary>
        /// Close detail command
        /// </summary>
        public ICommand? CloseDetail { get; }

        /// <summary>
        /// Show in IL command
        /// </summary>
        public ICommand? ShowInIL { get; }

        /// <summary>
        /// Is this model active?
        /// </summary>
        public bool IsActive
        {
            get => _isActive;
            set => this.RaiseAndSetIfChanged(ref _isActive, value);
        }

        /// <summary>
        /// Current location
        /// </summary>
        public NavigationLocation? NavigationLocation
        {
            get => _navigationLocation;
            set => this.RaiseAndSetIfChanged(ref _navigationLocation, value);
        }

        /// <summary>
        /// Assigned content
        /// </summary>
        public object? Content
        {
            get => _content;
            set
            {
                this.RaiseAndSetIfChanged(ref _content, value);
                
                if (value != null)
                {
                    OnObjectChanged();
                }
            }
        }

        /// <summary>
        /// Marker canvas view model
        /// </summary>
        public SourceObjectMarkerCanvasViewModel MarkerCanvasViewModel { get; } = new();

        /// <summary>
        /// Is the overlay visible?
        /// </summary>
        public bool IsOverlayVisible()
        {
            return SelectedFileViewModel != null;
        }

        /// <summary>
        /// Is a location visible?
        /// </summary>
        public bool IsLocationVisible(ShaderLocation location)
        {
            // Only shader files care about visibility
            if (SelectedFileViewModel is ShaderFileViewModel shaderFileViewModel)
            {
                return location.FileUID == shaderFileViewModel.UID;
            }

            return true;
        }

        /// <summary>
        /// Transform a shader location
        /// </summary>
        public int TransformLine(ShaderLocation shaderLocation)
        {
            return shaderLocation.Line;
        }

        /// <summary>
        /// Transform a shader line
        /// </summary>
        public ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel>? TransformInstructionLine(AssembledInstructionMapping mapping)
        {
            ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel> multiViewModel = new();
            
            // Subscribe to all shaders
            this.GetObservableShaders()?
                .ToObservableChangeSet()
                .OnItemAdded(shaderViewModel =>
                {
                    // Association is non-trivial, we need to query it
                    if (_propertyCollection?.GetService<IShaderInstructionMappingService>() is { } service)
                    {
                        multiViewModel.Associations.Add(
                            new ShaderMultiAssociationPair<ShaderInstructionSourceAssociationViewModel>
                            {
                                ShaderViewModel = shaderViewModel,
                                Association = service.GetOrCreateSourceAssociation(shaderViewModel.GUID, mapping)
                            });
                    }
                })
                .Subscribe();

            return multiViewModel;
        }

        /// <summary>
        /// Transform a shader location line
        /// </summary>
        public ShaderMultiAssociationViewModel<ShaderInstructionAssociationViewModel>? TransformSourceLine(int line)
        {
            if (_propertyCollection?.GetService<IShaderInstructionMappingService>() is not { } service)
            {
                return null;
            }
            
            ShaderMultiAssociationViewModel<ShaderInstructionAssociationViewModel> multiViewModel = new();
            
            // Subscribe to all shaders
            this.GetObservableShaders()?
                .ToObservableChangeSet()
                .OnItemAdded(shaderViewModel =>
                {
                    // Try to find the best match
                    ShaderFileViewModel? fileViewModel = ShaderPathUtils.FindBestMatchContainedFile(shaderViewModel, _selectedFileViewModel!.Filename);
                    
                    multiViewModel.Associations.Add(
                        new ShaderMultiAssociationPair<ShaderInstructionAssociationViewModel>
                        {
                            ShaderViewModel = shaderViewModel,
                            Association = service.GetOrCreateInstructionAssociation(
                                new ShaderShaderInstructionAssociationLocation()
                                {
                                    SGUID = shaderViewModel.GUID,
                                    FileUID = (int)(fileViewModel?.UID ?? 0),
                                    Line = line
                                })
                        });
                })
                .Subscribe();

            return multiViewModel;
        }

        /// <summary>
        /// Selected file
        /// </summary>
        public CodeFileViewModel? SelectedFileViewModel
        {
            get => _selectedFileViewModel;
            set => this.RaiseAndSetIfChanged(ref _selectedFileViewModel, value);
        }

        public CodeContentViewModel()
        {
            OnSelected = ReactiveCommand.Create(OnParentSelected);
            CloseDetail = ReactiveCommand.Create(OnCloseDetail);
            ShowInIL = ReactiveCommand.Create(OnShowInIL);
        }

        /// <summary>
        /// Invoked on parent selection
        /// </summary>
        private void OnParentSelected()
        {
            // If a shader file, bind the selection
            if (Content is ShaderViewModel shaderViewModel && _selectedFileViewModel is ShaderFileViewModel shaderFileViewModel)
            {
                if (ServiceRegistry.Get<IWorkspaceService>() is { } service)
                {
                    // Create navigation vm
                    service.SelectedShader = new ShaderNavigationViewModel()
                    {
                        Shader = shaderViewModel,
                        SelectedFile = shaderFileViewModel
                    };

                    // Bind selection
                    service.SelectedShader.WhenAnyValue(x => x.SelectedFile)
                        .Subscribe(x => SelectedFileViewModel = x);
                }
            }

            // If a code file, just assign it directly
            else if (Content is CodeFileViewModel codeFileViewModel)
            {
                SelectedFileViewModel = codeFileViewModel;
            }
        }

        /// <summary>
        /// Invoked on detail closes
        /// </summary>
        private void OnCloseDetail()
        {
            DetailViewModel = null;
        }

        /// <summary>
        /// Invoked on navigation requests
        /// </summary>
        private void OnShowInIL()
        {
            // Navigate to the currently selected validation object
            NavigationContext?.Navigate(typeof(ILShaderContentViewModel), SelectedTextualSourceObject?.Segment?.Location);
        }

        /// <summary>
        /// Invoked on object change
        /// </summary>
        private void OnObjectChanged()
        {
            // If a shader view model, bind things
            if (Content is ShaderViewModel shaderViewModel)
            {
                // Submit request if not already
                if (shaderViewModel!.Contents == string.Empty)
                {
                    PropertyCollection?.GetService<IShaderCodeService>()?.EnqueueShaderContents(shaderViewModel);
                }

                // Set VM when available
                shaderViewModel.FileViewModels.ToObservableChangeSet().OnItemAdded(x =>
                {
                    if (SelectedFileViewModel == null)
                    {
                        SelectedFileViewModel = x;
                    }
                }).Subscribe();
            }
        }

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IPropertyViewModel? _propertyCollection;

        /// <summary>
        /// Internal icon state
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("Code");

        /// <summary>
        /// Selected file
        /// </summary>
        private CodeFileViewModel? _selectedFileViewModel = null;

        /// <summary>
        /// Internal location
        /// </summary>
        private NavigationLocation? _navigationLocation;

        /// <summary>
        /// Internal active state
        /// </summary>
        private bool _isActive = false;
        
        /// <summary>
        /// Internal selections tate
        /// </summary>
        private ITextualSourceObject? _selectedSourceObject;

        /// <summary>
        /// Internal detail state
        /// </summary>
        private ISourceObjectDetailViewModel? _detailViewModel;

        /// <summary>
        /// Internal shader
        /// </summary>
        private ShaderViewModel? _shaderViewModel;

        /// <summary>
        /// Internal content
        /// </summary>
        private object? _content;
    }
}