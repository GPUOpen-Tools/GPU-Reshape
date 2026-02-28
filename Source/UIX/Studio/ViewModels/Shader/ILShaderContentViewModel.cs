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
using ReactiveUI;
using Runtime.Utils.Workspace;
using Runtime.ViewModels.IL;
using Runtime.ViewModels.Shader;
using Runtime.ViewModels.Traits;
using Studio.Models.Workspace.Objects;
using Studio.Services;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Properties;
using ShaderViewModel = Studio.ViewModels.Workspace.Objects.ShaderViewModel;

namespace Studio.ViewModels.Shader
{
    public class ILShaderContentViewModel : ReactiveObject, IShaderContentViewModel, ITextualContent
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
        public string? ToolTip => Resources.Resources.ShaderView_IL;

        /// <summary>
        /// The final assembled program
        /// </summary>
        public string AssembledProgram
        {
            get => _assembledProgram;
            set => this.RaiseAndSetIfChanged(ref _assembledProgram, value);
        }

        /// <summary>
        /// The current assembler
        /// </summary>
        public Assembler? Assembler
        {
            get => _assembler;
            set => this.RaiseAndSetIfChanged(ref _assembler, value);
        }

        /// <summary>
        /// Currently selected validation object
        /// </summary>
        public ITextualSourceObject? SelectedTextualSourceObject
        {
            get => _selectedSourceObject;
            set => this.RaiseAndSetIfChanged(ref _selectedSourceObject, value);
        }

        /// <summary>
        /// Marker canvas view model
        /// </summary>
        public SourceObjectMarkerCanvasViewModel MarkerCanvasViewModel { get; } = new();
        
        /// <summary>
        /// Current detail view model
        /// </summary>
        public ISourceObjectDetailViewModel? DetailViewModel
        {
            get => _detailViewModel;
            set => this.RaiseAndSetIfChanged(ref _detailViewModel, value);
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
                // Subscribe before potential listeners kick off
                if (value != null)
                {
                    OnObjectChanged((ShaderViewModel)value);
                }
                
                this.RaiseAndSetIfChanged(ref _content, value);
            }
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
        /// Show in source command
        /// </summary>
        public ICommand? ShowInSource { get; }
        
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
        /// Shader view model of the content
        /// </summary>
        public ShaderViewModel? ShaderViewModel => Content as ShaderViewModel;

        /// <summary>
        /// Is this model active?
        /// </summary>
        public bool IsActive
        {
            get => _isActive;
            set => this.RaiseAndSetIfChanged(ref _isActive, value);
        }
        
        public ILShaderContentViewModel()
        {
            OnSelected = ReactiveCommand.Create(OnParentSelected);
            CloseDetail = ReactiveCommand.Create(OnCloseDetail);
            ShowInSource = ReactiveCommand.Create(OnShowInSource);
        }

        /// <summary>
        /// Invoked on parent selection
        /// </summary>
        private void OnParentSelected()
        {
            if (ServiceRegistry.Get<IWorkspaceService>() is { } service)
            {
                // Create navigation vm
                service.SelectedShader = new ShaderNavigationViewModel()
                {
                    Shader = ShaderViewModel,
                    SelectedFile = null
                };
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
        private void OnShowInSource()
        {
            // Navigate to the currently selected validation object
            NavigationContext?.Navigate(typeof(CodeContentViewModel), SelectedTextualSourceObject?.Segment?.Location);
        }

        /// <summary>
        /// Is the overlay visible?
        /// </summary>
        /// <returns></returns>
        public bool IsOverlayVisible()
        {
            return true;
        }

        /// <summary>
        /// Is a location visible?
        /// </summary>
        public bool IsLocationVisible(ShaderLocation location)
        {
            return true;
        }

        /// <summary>
        /// Transform a shader location
        /// </summary>
        public int TransformLine(ShaderLocation shaderLocation)
        {
            if (Assembler == null)
            {
                throw new Exception("Transformation without an assembler");
            }
            
            // Transform instruction indices to line from assembler
            return (int)Assembler.GetLineMapping(shaderLocation.BasicBlockId, shaderLocation.InstructionIndex).Line;
        }

        /// <summary>
        /// Transform a shader line
        /// </summary>
        public ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel>? TransformInstructionLine(AssembledInstructionMapping mapping)
        {
            ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel> viewModel = new();

            // Association is trivial, just fetch the assembled lookup
            ShaderUtils.SubscribeDeferredProgram(PropertyCollection!, ShaderViewModel!, () =>
            {
                viewModel.Associations.Add(
                    new ShaderMultiAssociationPair<ShaderInstructionSourceAssociationViewModel>()
                    {
                        ShaderViewModel = ShaderViewModel!,
                        Association = new ShaderInstructionSourceAssociationViewModel()
                        {
                            Location = new ShaderLocation()
                            {
                                BasicBlockId = mapping.BasicBlockId,
                                InstructionIndex = mapping.InstructionIndex,
                                Line = TransformLine(new ShaderLocation()
                                {
                                    BasicBlockId = mapping.BasicBlockId,
                                    InstructionIndex = mapping.InstructionIndex
                                })
                            }
                        }
                    }
                );
            });

            return viewModel;
        }

        /// <summary>
        /// Transform a shader location line
        /// </summary>
        public ShaderMultiAssociationViewModel<ShaderInstructionAssociationViewModel>? TransformSourceLine(int line)
        {
            if (Assembler == null)
            {
                throw new Exception("Transformation without an assembler");
            }
            
            ShaderMultiAssociationViewModel<ShaderInstructionAssociationViewModel> multiViewModel = new();
            
            // Transform instruction indices to line from assembler
            multiViewModel.Associations.Add(new ShaderMultiAssociationPair<ShaderInstructionAssociationViewModel>
            {
                ShaderViewModel = ShaderViewModel!,
                Association = new ShaderInstructionAssociationViewModel
                {
                    Mappings =
                    {
                        Assembler.GetInstructionMapping(new AssembledLineMapping()
                        {
                            Line = (uint)line
                        })
                    },
                    Populated = true
                }
            });
            
            return multiViewModel;
        }

        /// <summary>
        /// Invoked on object change
        /// </summary>
        private void OnObjectChanged(ShaderViewModel content)
        {
            // Submit request if not already
            if (content.Program == null)
            {
                PropertyCollection?.GetService<IShaderCodeService>()?.EnqueueShaderIL(content);
            }

            // Bind program, assemble when changed
            content.WhenAnyValue(x => x.Program).WhereNotNull().Subscribe(program =>
            {
                // Create assembler
                _assembler = new Assembler(program);
                
                // Assembler used assemble!
                AssembledProgram = _assembler.Assemble();
            });
        }

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IPropertyViewModel? _propertyCollection;

        /// <summary>
        /// Internal icon state
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("CommandCode");

        /// <summary>
        /// Internal active state
        /// </summary>
        private bool _isActive = false;

        /// <summary>
        /// Internal location
        /// </summary>
        private NavigationLocation? _navigationLocation;

        /// <summary>
        /// Internal assembler
        /// </summary>
        private Assembler? _assembler;

        /// <summary>
        /// Internal assembled program
        /// </summary>
        private string _assembledProgram;

        /// <summary>
        /// Internal selection state
        /// </summary>
        private ITextualSourceObject? _selectedSourceObject;

        /// <summary>
        /// Internal detail state
        /// </summary>
        private ISourceObjectDetailViewModel? _detailViewModel;

        /// <summary>
        /// Internal content
        /// </summary>
        private object? _content;
    }
}