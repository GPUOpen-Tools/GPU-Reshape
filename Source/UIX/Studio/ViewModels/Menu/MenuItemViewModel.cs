using System.Collections.ObjectModel;
using System.Windows.Input;

namespace Studio.ViewModels.Menu
{
    public class MenuItemViewModel : IMenuItemViewModel
    {
        /// <summary>
        /// Given header
        /// </summary>
        public string Header { get; set; } = string.Empty;

        /// <summary>
        /// All hosted items
        /// </summary>
        public ObservableCollection<IMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Command on invoke
        /// </summary>
        public ICommand? Command { get; set; }

        /// <summary>
        /// If this context menu is enabled
        /// </summary>
        public bool IsEnabled { get; set; } = true;
    }
}