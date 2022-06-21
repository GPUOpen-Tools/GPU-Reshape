using System.Collections.ObjectModel;
using System.Windows.Input;

namespace Studio.ViewModels.Contexts
{
    public interface IContextMenuItem
    {
        /// <summary>
        /// Display header of this menu item
        /// </summary>
        public string Header { get; set; }
        
        /// <summary>
        /// Items within this menu item
        /// </summary>
        public ObservableCollection<IContextMenuItem> Items { get; }
        
        /// <summary>
        /// Target command
        /// </summary>
        public ICommand Command { get; }
        
        /// <summary>
        /// Is this menu item enabled?
        /// </summary>
        public bool IsEnabled { get; set; }
        
        /// <summary>
        /// Target view model of the context
        /// </summary>
        public object? TargetViewModel { get; set; }
    }
}