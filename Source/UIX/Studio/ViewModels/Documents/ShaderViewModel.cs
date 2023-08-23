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
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive;
using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Services;

namespace Studio.ViewModels.Documents
{
    public class ShaderViewModel : Document, IDocumentViewModel, INavigationContext
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
                if (value is not ShaderDescriptor { } descriptor)
                {
                    return;
                }

                // Construction descriptor?
                if (_descriptor == null)
                {
                    ConstructDescriptor(descriptor);
                }
                
                // Update content descriptors
                ShaderContentViewModels.ForEach(x => x.Descriptor = descriptor);
                
                _descriptor = descriptor;
            }
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
        /// Current content view model
        /// </summary>
        public IShaderContentViewModel? SelectedShaderContentViewModel
        {
            get => _selectedShaderContentViewModel;
            set => this.RaiseAndSetIfChanged(ref _selectedShaderContentViewModel, value);
        }

        /// <summary>
        /// Is the object ready?
        /// </summary>
        public bool Ready
        {
            get => _ready;
            set => this.RaiseAndSetIfChanged(ref _ready, value);
        }
        
        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public IPropertyViewModel? PropertyCollection
        {
            get => _propertyCollection;
            set => this.RaiseAndSetIfChanged(ref _propertyCollection, value);
        }

        /// <summary>
        /// Shader UID
        /// </summary>
        public UInt64 GUID
        {
            get => _guid;
            set
            {
                this.RaiseAndSetIfChanged(ref _guid, value);
                OnShaderChanged();
            }
        }

        /// <summary>
        /// Underlying object
        /// </summary>
        public Workspace.Objects.ShaderViewModel? Object
        {
            get => _object;
            set
            {
                this.RaiseAndSetIfChanged(ref _object, value);
                
                if (_object != null)
                {
                    OnObjectChanged();
                }
            }
        }

        /// <summary>
        /// All view models
        /// </summary>
        public ObservableCollection<IShaderContentViewModel> ShaderContentViewModels { get; } = new();

        /// <summary>
        /// Icon color
        /// </summary>
        public Color? IconForeground
        {
            get => _iconForeground;
            set => this.RaiseAndSetIfChanged(ref _iconForeground, value);
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public ShaderViewModel()
        {
            // Binding
            ShaderContentViewModels.ToObservableChangeSet(x => x)
                .OnItemAdded(shaderContentViewModel =>
                {
                    // Bind properties
                    this.BindProperty(x => x.PropertyCollection, x => shaderContentViewModel.PropertyCollection = x);
                    this.BindProperty(x => x.Object, x => shaderContentViewModel.Object = x);

                    // Bind active state change
                    shaderContentViewModel
                        .WhenAnyValue(x => x.IsActive)
                        .Subscribe(x => OnSetContentViewModel(shaderContentViewModel, x));
                })
                .Subscribe();
            
            // Default view models
            ShaderContentViewModels.AddRange(new IShaderContentViewModel[]
            {
                new CodeShaderContentViewModel()
                {
                    NavigationContext = this
                },
                new ILShaderContentViewModel()
                {
                    NavigationContext = this
                },
                new BlockGraphShaderContentViewModel()
                {
                    NavigationContext = this
                }
            });
            
            // Selected
            ShaderContentViewModels[0].IsActive = true;
        }

        /// <summary>
        /// Construct from a given descriptor
        /// </summary>
        /// <param name="descriptor"></param>
        private void ConstructDescriptor(ShaderDescriptor descriptor)
        {
            Id = $"Shader{descriptor.GUID}";
            Title = $"Loading ...";
            PropertyCollection = descriptor.PropertyCollection;
            GUID = descriptor.GUID;
        }

        /// <summary>
        /// Invoked on selection
        /// </summary>
        public override void OnSelected()
        {
            base.OnSelected();
            
            // Proxy to current selection
            SelectedShaderContentViewModel?.OnSelected?.Execute(Unit.Default);
        }

        /// <summary>
        /// Invoked on object changes
        /// </summary>
        private void OnObjectChanged()
        {
            _object!.WhenAnyValue(x => x.Filename).Subscribe(x =>
            {
                if (x == string.Empty)
                {
                    Title = $"Shader ({_object.GUID})";
                }
                else
                {
                    Title = $"{System.IO.Path.GetFileName(_object!.Filename)} ({_object.GUID})";
                }
            });

            // Bind async status
            _object!.WhenAnyValue(x => x.AsyncStatus).Subscribe(x =>
            {
                // Ready?
                Ready = x != AsyncShaderStatus.Pending;

                // Switch to IL content view if there's no debug symbols
                if (x == AsyncShaderStatus.NoDebugSymbols)
                {
                    SelectedShaderContentViewModel = ShaderContentViewModels.First(scvm => scvm is ILShaderContentViewModel);
                }
            });
        }

        private void OnSetContentViewModel(IShaderContentViewModel shaderContentViewModel, bool state)
        {
            // Note: There's a binding bug somewhere with IsEnabled = !IsActive, so, this is a workaround
            if (!state)
            {
                if (SelectedShaderContentViewModel == shaderContentViewModel)
                {
                    shaderContentViewModel.IsActive = true;
                }
                return;
            }

            // Update selected
            SelectedShaderContentViewModel = shaderContentViewModel;
            
            // Disable rest
            ShaderContentViewModels.Where(x => x != shaderContentViewModel).ForEach(x => x.IsActive = false);
        }

        /// <summary>
        /// Invoked when the shader has changed
        /// </summary>
        public void OnShaderChanged()
        {
            // Get collection
            var collection = _propertyCollection?.GetProperty<ShaderCollectionViewModel>();

            // Fetch object
            Object = collection?.GetOrAddShader(_guid);
        }

        /// <summary>
        /// Invoked on navigation requests
        /// </summary>
        public void Navigate(object target, object? parameter)
        {
            IShaderContentViewModel? targetViewModel;

            // Typed navigation?
            if (target is Type type)
            {
                targetViewModel = ShaderContentViewModels.FirstOrDefault(x => type.IsInstanceOfType(x));
            }
            else
            {
                targetViewModel = target as IShaderContentViewModel;
            }

            // None found?
            if (targetViewModel == null)
            {
                return;
            }

            // Assign navigation location if needed
            if (parameter != null)
            {
                targetViewModel.NavigationLocation = (ShaderLocation)parameter;
            }

            // Set new target
            SelectedShaderContentViewModel = targetViewModel;
        }

        /// <summary>
        /// Internal guid
        /// </summary>
        private UInt64 _guid;

        /// <summary>
        /// Internal selection state
        /// </summary>
        private IShaderContentViewModel? _selectedShaderContentViewModel;

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IPropertyViewModel? _propertyCollection;

        /// <summary>
        /// Internal object
        /// </summary>
        private Workspace.Objects.ShaderViewModel? _object;
        
        /// <summary>
        /// Internal icon
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("DotsGrid");

        /// <summary>
        /// Internal descriptor
        /// </summary>
        private ShaderDescriptor? _descriptor;

        /// <summary>
        /// Internal icon color
        /// </summary>
        private Color? _iconForeground = ResourceLocator.GetResource<Color>("ThemeForegroundColor");

        /// <summary>
        /// Internal ready state
        /// </summary>
        private bool _ready;
    }
}