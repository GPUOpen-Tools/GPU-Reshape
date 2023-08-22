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
using System.Windows.Input;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
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
            Objects.ToObservableChangeSet()
                .OnItemAdded(ObjectsChanged)
                .OnItemRemoved(ObjectsChanged)
                .Subscribe();
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
    }
}