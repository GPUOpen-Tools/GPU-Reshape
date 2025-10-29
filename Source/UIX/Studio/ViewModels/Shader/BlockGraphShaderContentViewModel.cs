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

using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia.Media;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Runtime.ViewModels.Traits;
using Studio.Services;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Properties;
using ShaderViewModel = Studio.ViewModels.Workspace.Objects.ShaderViewModel;

namespace Studio.ViewModels.Shader
{
    public class BlockGraphShaderContentViewModel : ReactiveObject, IShaderContentViewModel
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
                
            }
        }

        /// <summary>
        /// Selection command
        /// </summary>
        public ICommand? OnSelected { get; }
        
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
        public string? ToolTip => Resources.Resources.ShaderView_BlockGraph;
        
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
                
                if (_content != null)
                {
                    OnObjectChanged();
                }
            }
        }

        /// <summary>
        /// Shader view model of the content
        /// </summary>
        public ShaderViewModel? ShaderViewModel => Content as ShaderViewModel;

        public BlockGraphShaderContentViewModel()
        {
            OnSelected = ReactiveCommand.Create(OnParentSelected);
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
        /// Is the overlay visible?
        /// </summary>
        public bool IsOverlayVisible()
        {
            return true;
        }

        /// <summary>
        /// Is a validation object visible?
        /// </summary>
        public bool IsObjectVisible(ValidationObject validationObject)
        {
            return true;
        }

        /// <summary>
        /// Invoked on object changes
        /// </summary>
        private void OnObjectChanged()
        {
            // Submit request if not already
            if (ShaderViewModel!.BlockGraph == string.Empty)
            {
                PropertyCollection?.GetService<IShaderCodeService>()?.EnqueueShaderBlockGraph(ShaderViewModel);
            }
        }

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IPropertyViewModel? _propertyCollection;

        /// <summary>
        /// Internal icon state
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("DotGraph");

        /// <summary>
        /// Internal location
        /// </summary>
        private NavigationLocation? _navigationLocation;

        /// <summary>
        /// Internal active state
        /// </summary>
        private bool _isActive = false;

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