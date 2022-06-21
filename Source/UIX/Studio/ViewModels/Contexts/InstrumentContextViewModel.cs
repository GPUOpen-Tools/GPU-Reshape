using System.Collections.ObjectModel;
using System.Reactive;
using System.Windows.Input;
using ReactiveUI;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Contexts
{
    public class InstrumentContextViewModel : ReactiveObject, IContextMenuItem
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
                IsEnabled = _targetViewModel is IPropertyViewModel;
            }
        }

        /// <summary>
        /// Display header of this context model
        /// </summary>
        public string Header { get; set; } = "Instrument";
        
        /// <summary>
        /// All items within this context model
        /// </summary>
        public ObservableCollection<IContextMenuItem> Items { get; } = new();

        /// <summary>
        /// Target command
        /// </summary>
        public ICommand Command { get; }

        /// <summary>
        /// Is this context enabled?
        /// </summary>
        public bool IsEnabled
        {
            get => _isEnabled;
            set => this.RaiseAndSetIfChanged(ref _isEnabled, value);
        }

        public InstrumentContextViewModel()
        {
            Command = ReactiveCommand.Create(OnInvoked);
            
            // WILL BE MOVED TO MODULAR APPROACH
            Items.Add(new Instrument.ResourceBoundsContextViewModel());
        }

        /// <summary>
        /// Command implementation
        /// </summary>
        private void OnInvoked()
        {
            
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