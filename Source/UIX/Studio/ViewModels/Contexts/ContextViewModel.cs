using System.Collections.ObjectModel;
using ReactiveUI;

namespace Studio.ViewModels.Contexts
{
    public class ContextViewModel : ReactiveObject, IContextViewModel
    {
        /// <summary>
        /// Target view model of the context
        /// </summary>
        public object? TargetViewModel
        {
            get => _targetViewModel;
            set => this.RaiseAndSetIfChanged(ref _targetViewModel, value);
        }

        /// <summary>
        /// Items of this context
        /// </summary>
        public ObservableCollection<IContextMenuItem> Items { get; } = new();

        public ContextViewModel()
        {
            Items.Add(new InstrumentContextViewModel());
        }

        /// <summary>
        /// Internal target view model
        /// </summary>
        private object? _targetViewModel;
    }
}