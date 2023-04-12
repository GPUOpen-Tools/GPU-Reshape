﻿using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive.Linq;
using System.Windows.Input;
using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Documents
{
    public class ShaderViewModel : Document, IDocumentViewModel
    {
        /// <summary>
        /// Descriptor setter, constructs the top document from given descriptor
        /// </summary>
        public IDescriptor? Descriptor
        {
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
                new CodeShaderContentViewModel(),
                new ILShaderContentViewModel(),
                new BlockGraphShaderContentViewModel()
            });
            
            // Selected
            ShaderContentViewModels[0].IsActive = true;
        }

        /// <summary>
        /// Invoked on object changes
        /// </summary>
        private void OnObjectChanged()
        {
            _object!.WhenAnyValue(x => x.Filename).Where(x => x != string.Empty).Subscribe(x =>
            {
                Title = $"{_object!.Filename} ({_object.GUID})";
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
    }
}