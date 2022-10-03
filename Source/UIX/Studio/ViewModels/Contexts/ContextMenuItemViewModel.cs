using System.Collections.ObjectModel;
using System.Windows.Input;

namespace Studio.ViewModels.Contexts
{
    public class ContextMenuItemViewModel : IContextMenuItemViewModel
    {
        /// <summary>
        /// Given header
        /// </summary>
        public string Header { get; set; } = string.Empty;

        /// <summary>
        /// All hosted items
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Command on invoke
        /// </summary>
        public ICommand Command { get; }

        /// <summary>
        /// If this context menu is enabled
        /// </summary>
        public bool IsEnabled { get; set; } = true;
        
        /// <summary>
        /// Underlying target view model for the context menu
        /// </summary>
        public object? TargetViewModel { get; set; }
    }
}