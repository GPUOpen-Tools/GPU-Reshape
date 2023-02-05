using System.Collections.ObjectModel;
using System.Windows.Input;

namespace Studio.ViewModels.Setting
{
    public interface ISettingItemViewModel
    {
        /// <summary>
        /// Display header of this settings item
        /// </summary>
        public string Header { get; set; }
        
        /// <summary>
        /// Items within this settings item
        /// </summary>
        public ObservableCollection<ISettingItemViewModel> Items { get; }
        
        /// <summary>
        /// Is this setting item enabled?
        /// </summary>
        public bool IsEnabled { get; set; }
    }

    public static class SettingItemExtensions
    {
        /// <summary>
        /// Get an item from this context settings item
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetItem<T>(this ISettingItemViewModel self) where T : ISettingItemViewModel
        {
            foreach (ISettingItemViewModel settingItemViewModel in self.Items)
            {
                if (settingItemViewModel is T typed)
                {
                    return typed;
                }
            }

            return default;
        }
    }
}