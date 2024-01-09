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
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Controls
{
    public class ValidationMarkerViewModel : ReactiveObject
    {
        /// <summary>
        /// Transformed source line
        /// </summary>
        public int SourceLine { get; set; }

        /// <summary>
        /// Is this marker selected?
        /// </summary>
        public bool IsSelected
        {
            get => _isSelected;
            set => this.RaiseAndSetIfChanged(ref _isSelected, value);
        }

        /// <summary>
        /// The textual view model
        /// </summary>
        public ITextualShaderContentViewModel? ShaderContentViewModel
        {
            get => _shaderContentViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _shaderContentViewModel, value);
                OnShaderContentChanged();
            }
        }

        /// <summary>
        /// The command to be invoked on details
        /// </summary>
        public ICommand? DetailCommand
        {
            get => _detailCommand;
            set => this.RaiseAndSetIfChanged(ref _detailCommand, value);
        }
        
        /// <summary>
        /// Currently selected object
        /// </summary>
        public ValidationObject? SelectedObject
        {
            get => _selectedObject;
            set => this.RaiseAndSetIfChanged(ref _selectedObject, value);
        }

        /// <summary>
        /// Is the flyout visible?
        /// </summary>
        public bool IsFlyoutVisible
        {
            get => _isFlyoutVisible;
            set => this.RaiseAndSetIfChanged(ref _isFlyoutVisible, value);
        }

        /// <summary>
        /// All validation objects
        /// </summary>
        public ObservableCollection<ValidationObject> Objects { get; } = new();

        /// <summary>
        /// Constructor
        /// </summary>
        public ValidationMarkerViewModel()
        {
            // Make observable
            Objects.ToObservableChangeSet()
                .OnItemAdded(ObjectsChanged)
                .OnItemRemoved(ObjectsChanged)
                .Subscribe();

            // Bind selection
            this.WhenAnyValue(x => x.SelectedObject)
                .Subscribe(x => OnLocalSelectionChanged());
        }

        /// <summary>
        /// Invoked on content changes
        /// </summary>
        private void OnShaderContentChanged()
        {
            // Bind selection
            _shaderContentViewModel?
                .WhenAnyValue(x => x.SelectedValidationObject)
                .Subscribe(x => UpdateSelection());
        }

        /// <summary>
        /// Invoked on local selection changes
        /// </summary>
        private void OnLocalSelectionChanged()
        {
            if (_shaderContentViewModel == null)
            {
                return;
            }
            
            // Set on content view model
            _shaderContentViewModel.SelectedValidationObject = SelectedObject;
        }

        /// <summary>
        /// Invoked on content selection changes
        /// </summary>
        private void UpdateSelection()
        {
            // Get the selected object
            ValidationObject? selected = _shaderContentViewModel?.SelectedValidationObject;
            
            // Check if any local object satisifies it
            foreach (ValidationObject validationObject in Objects)
            {
                if (validationObject == selected)
                {
                    SelectedObject = validationObject;
                    IsSelected = true;
                    return;
                }
            }

            // None of our objects were selected
            IsSelected = false;
        }

        /// <summary>
        /// Invoked on object changes
        /// </summary>
        private void ObjectsChanged(ValidationObject _)
        {
            // Show the flyout if there's more than 1 object
            IsFlyoutVisible = Objects.Count > 1;
        }

        /// <summary>
        /// Internal selection state
        /// </summary>
        private ValidationObject? _selectedObject;
        
        /// <summary>
        /// Internal flyout state
        /// </summary>
        private bool _isFlyoutVisible;
        
        /// <summary>
        /// Internal detail state
        /// </summary>
        private ICommand? _detailCommand;

        /// <summary>
        /// Internal content view model
        /// </summary>
        private ITextualShaderContentViewModel? _shaderContentViewModel;
        
        /// <summary>
        /// Internal selection state
        /// </summary>
        private bool _isSelected;
    }
}