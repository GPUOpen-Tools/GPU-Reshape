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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia;
using Avalonia.Threading;
using DynamicData;
using DynamicData.Binding;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Workspace.Listeners;
using Studio.Models.Workspace.Objects;
using Studio.Services;
using Studio.ViewModels.Documents;
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
        public PropertyVisibility Visibility => PropertyVisibility.WorkspaceOverview;

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
        /// Filtered condensed messages
        /// </summary>
        public ObservableCollection<Objects.ValidationObject> FilteredValidationObjects { get; } = new();

        /// <summary>
        /// Hide all validation objects with missing symbols?
        /// </summary>
        public bool HideMissingSymbols
        {
            get => _hideMissingSymbols;
            set => this.RaiseAndSetIfChanged(ref _hideMissingSymbols, value);
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
        /// Clear all messages
        /// </summary>
        public ICommand Clear { get; }

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
            Clear = ReactiveCommand.Create(OnClear);

            // Bind objects for filtering
            ValidationObjects
                .ToObservableChangeSet()
                .OnItemAdded(OnValidationItemAdded)
                .OnItemRemoved(OnValidationItemRemoved)
                .Subscribe();
        }

        /// <summary>
        /// Invoked on missing symbol toggle
        /// </summary>
        private void OnToggleHideMissingSymbols()
        {
            HideMissingSymbols = !HideMissingSymbols;
            ResummarizeFilter();
        }

        /// <summary>
        /// Invoked when an object was added
        /// </summary>
        private void OnValidationItemAdded(ValidationObject obj)
        {
            FilterMessageChained(obj);
        }

        /// <summary>
        /// Invoked when an object was removed
        /// </summary>
        private void OnValidationItemRemoved(ValidationObject obj)
        {
            FilteredValidationObjects.Remove(obj);
        }

        /// <summary>
        /// Invoked on validation object clearing
        /// </summary>
        private void OnClear()
        {
            // Flush both
            ValidationObjects.Clear();
            FilteredValidationObjects.Clear();
        }

        /// <summary>
        /// Resummarize all validation objects
        /// </summary>
        private void ResummarizeFilter()
        {
            // Force flush
            FilteredValidationObjects.Clear();

            // Filter all messages
            foreach (ValidationObject validationObject in ValidationObjects)
            {
                FilterMessageChained(validationObject);
            }
        }
        
        /// <summary>
        /// Filter a validation object
        /// </summary>
        private void FilterMessageChained(ValidationObject validationObject)
        {
            // Filter by symbols?
            if (HideMissingSymbols && string.IsNullOrEmpty(validationObject.Extract))
            {
                // Bind on segment, re-queue filter when successful
                validationObject.WhenAnyValue(x => x.Segment).Subscribe(x =>
                {
                    if (!string.IsNullOrEmpty(validationObject.Extract))
                    {
                        FilterMessageChained(validationObject);
                    }
                });
                
                // Chain down
                return;
            }
            
            // Passed
            FilteredValidationObjects.Add(validationObject);
        }

        /// <summary>
        /// On shader document open
        /// </summary>
        /// <param name="viewModel">must be ValidationObject</param>
        private void OnOpenShaderDocument(object viewModel)
        {
            var validationObject = (ValidationObject)viewModel;

            // May not be ready yet
            if (validationObject.Segment == null)
            {
                return;
            }

            // Create document
            if (App.Locator.GetService<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.DocumentLayout?.OpenDocument(new ShaderDescriptor()
                {
                    PropertyCollection = Parent,
                    StartupLocation = validationObject.Segment.Location,
                    GUID = validationObject.Segment.Location.SGUID
                });
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
        /// Internal missing symbol state
        /// </summary>
        private bool _hideMissingSymbols = false;
    }
}