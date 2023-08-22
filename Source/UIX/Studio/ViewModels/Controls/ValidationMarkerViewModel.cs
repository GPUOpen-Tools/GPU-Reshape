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