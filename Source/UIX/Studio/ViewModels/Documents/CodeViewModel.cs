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
using System.IO;
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
using Studio.Services;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Services;

namespace Studio.ViewModels.Documents
{
    public class CodeViewModel : Document, IDocumentViewModel, INavigationContext
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
                if (value is not CodeFileDescriptor { } descriptor)
                {
                    return;
                }
                
                // Construction descriptor?
                if (_descriptor == null)
                {
                    this.RaiseAndSetIfChanged(ref _descriptor, descriptor);
                    ConstructDescriptor(descriptor);
                }
            }
        }
        
        /// <summary>
        /// Current content view model
        /// </summary>
        public IContentViewModel? SelectedContentViewModel
        {
            get => _selectedContentViewModel;
            set => this.RaiseAndSetIfChanged(ref _selectedContentViewModel, value);
        }

        /// <summary>
        /// All view models
        /// </summary>
        public ObservableCollection<IContentViewModel> ContentViewModels { get; } = new();

        /// <summary>
        /// Document icon
        /// </summary>
        public StreamGeometry? Icon
        {
            get => _icon;
            set => this.RaiseAndSetIfChanged(ref _icon, value);
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
        /// Icon color
        /// </summary>
        public IBrush? IconForeground
        {
            get => _iconForeground;
            set => this.RaiseAndSetIfChanged(ref _iconForeground, value);
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public CodeViewModel()
        {
            // Binding
            ContentViewModels
                .ToObservableChangeSet(x => x)
                .OnItemAdded(contentViewModel =>
                {
                    // Bind properties
                    this.BindProperty(x => x.PropertyCollection, x => contentViewModel.PropertyCollection = x);
                    this.BindProperty(x => x.Descriptor, x => OnFileChanged(contentViewModel, (CodeFileDescriptor)x!));

                    // Bind active state change
                    contentViewModel
                        .WhenAnyValue(x => x.IsActive)
                        .Subscribe(x => OnSetContentViewModel(contentViewModel, x));
                    
                    // Install the extensions
                    ServiceRegistry.Get<IEditorService>()?.InstallViewModel(contentViewModel);
                })
                .Subscribe();

            // Default view models
            ContentViewModels.AddRange(new IContentViewModel[]
            {
                new CodeContentViewModel()
                {
                    NavigationContext = this
                }
            });

            // Selected
            ContentViewModels[0].IsActive = true;
        }

        /// <summary>
        /// Construct from a given descriptor
        /// </summary>
        /// <param name="descriptor"></param>
        private void ConstructDescriptor(CodeFileDescriptor descriptor)
        {
            Id = descriptor.CodeFileViewModel.Filename;
            Title = Path.GetFileName(descriptor.CodeFileViewModel.Filename);
            PropertyCollection = descriptor.PropertyCollection;
        }

        /// <summary>
        /// Invoked on file changes
        /// </summary>
        private void OnFileChanged(IContentViewModel contentViewModel, CodeFileDescriptor? codeFileDescriptor)
        {
            if (codeFileDescriptor == null)
            {
                return;
            }
            
            // Lazy initialize file
            codeFileDescriptor.PropertyCollection?
                .GetService<IFileCodeService>()?
                .InstantiateWithWatch(codeFileDescriptor.CodeFileViewModel);

            // Set content to file
            contentViewModel.Content = codeFileDescriptor.CodeFileViewModel;
        }

        /// <summary>
        /// Invoked on content changes
        /// </summary>
        private void OnSetContentViewModel(IContentViewModel shaderContentViewModel, bool state)
        {
            // Note: There's a binding bug somewhere with IsEnabled = !IsActive, so, this is a workaround
            if (!state)
            {
                if (SelectedContentViewModel == shaderContentViewModel)
                {
                    shaderContentViewModel.IsActive = true;
                }
                return;
            }

            // Update selected
            SelectedContentViewModel = shaderContentViewModel;
            
            // Disable rest
            ContentViewModels.Where(x => x != shaderContentViewModel).ForEach(x => x.IsActive = false);
        }

        /// <summary>
        /// Invoked on selection
        /// </summary>
        public override void OnSelected()
        {
            base.OnSelected();
            
            // Proxy to current selection
            SelectedContentViewModel?.OnSelected?.Execute(Unit.Default);
        }

        /// <summary>
        /// Invoked on navigation requests
        /// </summary>
        public void Navigate(object target, object? parameter)
        {
            IContentViewModel? targetViewModel;

            // Typed navigation?
            if (target is Type type)
            {
                targetViewModel = ContentViewModels.FirstOrDefault(x => type.IsInstanceOfType(x));
            }
            else
            {
                targetViewModel = target as IContentViewModel;
            }

            // None found?
            if (targetViewModel == null)
            {
                return;
            }

            // Assign navigation location if needed
            if (parameter != null)
            {
                targetViewModel.NavigationLocation = new NavigationLocation()
                {
                    Location = (ShaderLocation)parameter
                };
            }

            // Set new target
            SelectedContentViewModel = targetViewModel;
        }

        /// <summary>
        /// Internal guid
        /// </summary>
        private UInt64 _guid;

        /// <summary>
        /// Internal selection state
        /// </summary>
        private IContentViewModel? _selectedContentViewModel;

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IPropertyViewModel? _propertyCollection;

        /// <summary>
        /// Internal icon
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("DotsGrid");

        /// <summary>
        /// Internal descriptor
        /// </summary>
        private CodeFileDescriptor? _descriptor;

        /// <summary>
        /// Internal icon color
        /// </summary>
        private IBrush? _iconForeground = new SolidColorBrush(ResourceLocator.GetResource<Color>("SystemBaseHighColor"));

        /// <summary>
        /// Internal ready state
        /// </summary>
        private bool _ready;
    }
}