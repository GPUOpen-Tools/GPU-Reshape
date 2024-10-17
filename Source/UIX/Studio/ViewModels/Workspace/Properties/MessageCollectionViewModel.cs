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
using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Threading;
using DynamicData;
using DynamicData.Binding;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.Services;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Message;
using Studio.ViewModels.Workspace.Services;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Properties
{
    public class MessageCollectionViewModel : ReactiveObject, IMessageCollectionViewModel
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name { get; } = "Messages";

        /// <summary>
        /// Visibility of this property
        /// </summary>
        public PropertyVisibility Visibility => PropertyVisibility.WorkspaceOverview | PropertyVisibility.InlineSection;

        /// <summary>
        /// Parent property
        /// </summary>
        public IPropertyViewModel? Parent { get; set; }

        /// <summary>
        /// Child properties
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; set; } = new SourceList<IPropertyViewModel>();

        /// <summary>
        /// All services
        /// </summary>
        public ISourceList<IPropertyService> Services { get; set; } = new SourceList<IPropertyService>();

        /// <summary>
        /// All condensed messages
        /// </summary>
        public ObservableCollection<Objects.ValidationObject> ValidationObjects { get; } = new();

        /// <summary>
        /// Hierarchical query filter
        /// </summary>
        public HierarchicalMessageQueryFilterViewModel? HierarchicalMessageQueryFilterViewModel { get; } = new();

        /// <summary>
        /// Hierarchical message representation
        /// </summary>
        public HierarchicalMessageFilterViewModel HierarchicalMessageFilterViewModel { get; } = new();

        /// <summary>
        /// Currently selected item
        /// </summary>
        public IObservableTreeItem? SelectedItem
        {
            get => _selectedItem;
            set => this.RaiseAndSetIfChanged(ref _selectedItem, value);
        }

        /// <summary>
        /// Current scrolling offset
        /// </summary>
        public Vector ScrollAmount
        {
            get => _scrollAmount;
            set => this.RaiseAndSetIfChanged(ref _scrollAmount, value);
        }

        /// <summary>
        /// Opens a given shader document from view model
        /// </summary>
        public ICommand OpenShaderDocument { get; }

        /// <summary>
        /// Toggle missing symbols filter
        /// </summary>
        public ICommand ToggleHideMissingSymbols { get; }

        /// <summary>
        /// Toggle hierarhical mode
        /// </summary>
        public ICommand ToggleMode { get; }

        /// <summary>
        /// Toggle message severity
        /// </summary>
        public ICommand ToggleSeverity { get; }

        /// <summary>
        /// Clear all messages
        /// </summary>
        public ICommand Clear { get; }
        
        /// <summary>
        /// Expand all items
        /// </summary>
        public ICommand Expand { get; }
        
        /// <summary>
        /// Collapse all items
        /// </summary>
        public ICommand Collapse { get; }

        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _connectionViewModel, value);

                OnConnectionChanged();
            }
        }

        public MessageCollectionViewModel()
        {
            // Create commands
            OpenShaderDocument = ReactiveCommand.Create<object>(OnOpenShaderDocument);
            ToggleHideMissingSymbols = ReactiveCommand.Create(OnToggleHideMissingSymbols);
            ToggleMode = ReactiveCommand.Create<HierarchicalMode>(OnToggleMode);
            ToggleSeverity = ReactiveCommand.Create<ValidationSeverity>(OnToggleSeverity);
            Expand = ReactiveCommand.Create(OnExpand);
            Collapse = ReactiveCommand.Create(OnCollapse);
            Clear = ReactiveCommand.Create(OnClear);

            // Bind objects for filtering
            HierarchicalMessageFilterViewModel.QueryFilterViewModel = HierarchicalMessageQueryFilterViewModel;
            HierarchicalMessageFilterViewModel.Bind(ValidationObjects);

            // If design time, create some dummy items for testing
            if (Design.IsDesignMode)
            {
                ValidationObjects.AddRange(new []
                {
                    new ValidationObject()
                    {
                        Content = "Failure A",
                        Severity = ValidationSeverity.Info,
                        Count = 5,
                        Segment = new ShaderSourceSegment()
                        {
                            Extract = "Extract A",
                            Location = new ShaderLocation()
                        }
                    },
                    new ValidationObject()
                    {
                        Content = "Failure B - XXXXXXX",
                        Severity = ValidationSeverity.Warning,
                        Count = 10,
                        Segment = new ShaderSourceSegment()
                        {
                            Extract = "Extract B",
                            Location = new ShaderLocation()
                        }
                    },
                    new ValidationObject()
                    {
                        Content = "Failure C - YYYY",
                        Severity = ValidationSeverity.Error,
                        Count = 5,
                        Segment = new ShaderSourceSegment()
                        {
                            Extract = "Extract C",
                            Location = new ShaderLocation()
                        }
                    }
                });
            }
        }

        /// <summary>
        /// On mode toggles
        /// </summary>
        private void OnToggleMode(HierarchicalMode mode)
        {
            HierarchicalMessageFilterViewModel.Mode ^= mode;
        }

        /// <summary>
        /// On severity toggles
        /// </summary>
        private void OnToggleSeverity(ValidationSeverity severity)
        {
            HierarchicalMessageFilterViewModel.Severity ^= severity;
        }

        /// <summary>
        /// Invoked on missing symbol toggle
        /// </summary>
        private void OnToggleHideMissingSymbols()
        {
            HierarchicalMessageFilterViewModel.HideMissingSymbols = !HierarchicalMessageFilterViewModel.HideMissingSymbols;
        }

        /// <summary>
        /// On expand all items
        /// </summary>
        private void OnExpand()
        {
            HierarchicalMessageFilterViewModel.Expand();
        }

        /// <summary>
        /// On collapse all times
        /// </summary>
        private void OnCollapse()
        {
            HierarchicalMessageFilterViewModel.Collapse();
        }

        /// <summary>
        /// Invoked on validation object clearing
        /// </summary>
        private void OnClear()
        {
            // Flush both
            HierarchicalMessageFilterViewModel.Clear();
            ValidationObjects.Clear();
        }

        /// <summary>
        /// On shader document open
        /// </summary>
        /// <param name="viewModel">must be ValidationObject</param>
        private void OnOpenShaderDocument(object viewModel)
        {
            IDescriptor? descriptor = null;
            
            // Valid shader category?
            if (viewModel is ObservableShaderCategoryItem { ShaderViewModel: { } } shaderCategoryItem)
            {
                descriptor = new ShaderDescriptor()
                {
                    PropertyCollection = Parent,
                    GUID = shaderCategoryItem.ShaderViewModel.GUID
                };
            }
            
            // Valid message?
            else if (viewModel is ObservableMessageItem { ValidationObject.Segment: { } } observableMessageItem)
            {
                descriptor = new ShaderDescriptor()
                {
                    PropertyCollection = Parent,
                    StartupLocation = NavigationLocation.FromObject(observableMessageItem.ValidationObject),
                    GUID = observableMessageItem.ValidationObject.Segment.Location.SGUID
                };
            }

            // No descriptor deduced?
            if (descriptor == null)
            {
                return;
            }

            // Create document
            if (ServiceRegistry.Get<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.DocumentLayout?.OpenDocument(descriptor);
            }
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            // Set connection
            _shaderMappingService.ConnectionViewModel = ConnectionViewModel;
            
            // Make visible
            Parent?.Services.Add(_shaderMappingService);
            
            // Register internal listeners
            _connectionViewModel?.Bridge?.Register(ShaderSourceMappingMessage.ID, _shaderMappingService);

            // Assign workspace collection to filter
            HierarchicalMessageFilterViewModel.PropertyViewModel = this.GetWorkspaceCollection();

            // Create all properties
            CreateProperties();
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Remove listeners
            _connectionViewModel?.Bridge?.Deregister(ShaderSourceMappingMessage.ID, _shaderMappingService);
        }

        /// <summary>
        /// Create all properties
        /// </summary>
        private void CreateProperties()
        {
            Properties.Clear();

            if (_connectionViewModel == null)
            {
                return;
            }
        }

        /// <summary>
        /// Shader mapping bridge listener
        /// </summary>
        private ShaderMappingService _shaderMappingService = new();

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Internal selection
        /// </summary>
        private IObservableTreeItem? _selectedItem;
        
        /// <summary>
        /// Internal scroll
        /// </summary>
        private Vector _scrollAmount = Vector.Zero;
    }
}