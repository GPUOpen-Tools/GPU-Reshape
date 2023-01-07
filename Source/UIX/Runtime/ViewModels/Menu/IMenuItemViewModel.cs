using System.Collections.ObjectModel;
using System.Windows.Input;

namespace Studio.ViewModels.Menu
{
    public interface IMenuItemViewModel
    {
        /// <summary>
        /// Display header of this menu item
        /// </summary>
        public string Header { get; set; }
        
        /// <summary>
        /// Items within this menu item
        /// </summary>
        public ObservableCollection<IMenuItemViewModel> Items { get; }
        
        /// <summary>
        /// Target command
        /// </summary>
        public ICommand? Command { get; }
        
        /// <summary>
        /// Is this menu item enabled?
        /// </summary>
        public bool IsEnabled { get; set; }
    }

    public static class MenuItemExtensions
    {
        /// <summary>
        /// Get an item from this context menu item
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetItem<T>(this IMenuItemViewModel self) where T : IMenuItemViewModel
        {
            foreach (IMenuItemViewModel menuItemViewModel in self.Items)
            {
                if (menuItemViewModel is T typed)
                {
                    return typed;
                }
            }

            return default;
        }
    }
}