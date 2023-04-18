using System.Collections.ObjectModel;
using System.Reactive;
using System.Windows.Input;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Contexts
{
    public class InstrumentNoneContextViewModel : ReactiveObject, IInstrumentContextViewModel
    {
        /// <summary>
        /// Target view model of the context
        /// </summary>
        public object? TargetViewModel
        {
            get => _targetViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _targetViewModel, value);
                IsVisible = _targetViewModel is IInstrumentableObject;
            }
        }

        /// <summary>
        /// Display header of this context model
        /// </summary>
        public string Header { get; set; } = "None";
        
        /// <summary>
        /// All items within this context model
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Target command
        /// </summary>
        public ICommand Command { get; }

        /// <summary>
        /// Is this context enabled?
        /// </summary>
        public bool IsVisible
        {
            get => _isEnabled;
            set => this.RaiseAndSetIfChanged(ref _isEnabled, value);
        }

        public InstrumentNoneContextViewModel()
        {
            Command = ReactiveCommand.Create(OnInvoked);
        }

        /// <summary>
        /// Command implementation
        /// </summary>
        private void OnInvoked()
        {
            if (_targetViewModel is not IPropertyViewModel propertyViewModel)
            {
                return;
            }

            // Close all instrumentation properties
            while (propertyViewModel.GetProperty<IInstrumentationProperty>() is { } instrumentationProperty)
            {
                (instrumentationProperty as IClosableObject)?.CloseCommand?.Execute(Unit.Default);
            }
        }

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = false;

        /// <summary>
        /// Internal target view model
        /// </summary>
        private object? _targetViewModel;
    }
}