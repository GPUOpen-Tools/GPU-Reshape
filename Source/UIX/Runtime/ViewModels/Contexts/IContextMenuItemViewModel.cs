using System.Collections.ObjectModel;
using System.Windows.Input;

namespace Studio.ViewModels.Contexts
{
    public interface IContextMenuItemViewModel
    {
        /// <summary>
        /// Display header of this menu item
        /// </summary>
        public string Header { get; set; }
        
        /// <summary>
        /// Items within this menu item
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; }
        
        /// <summary>
        /// Target command
        /// </summary>
        public ICommand? Command { get; }
        
        /// <summary>
        /// Is this menu item enabled?
        /// </summary>
        public bool IsEnabled { get; set; }
        
        /// <summary>
        /// Target view model of the context
        /// </summary>
        public object? TargetViewModel { get; set; }
    }

    public static class ContextMenuItemExtensions
    {
        /// <summary>
        /// Get an item from this context menu item
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetItem<T>(this IContextMenuItemViewModel self) where T : IContextMenuItemViewModel
        {
            foreach (IContextMenuItemViewModel contextMenuItemViewModel in self.Items)
            {
                if (contextMenuItemViewModel is T typed)
                {
                    return typed;
                }
            }

            return default;
        }
    }
}