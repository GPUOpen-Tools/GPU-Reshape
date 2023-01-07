using System.Collections.ObjectModel;
using Avalonia.Controls;
using DynamicData;
using Studio.ViewModels.Contexts;
using Studio.ViewModels.Menu;

namespace Studio.Services
{
    public class MenuService : IMenuService
    {
        /// <summary>
        /// Root menu view model
        /// </summary>
        public IMenuItemViewModel ViewModel { get; } = new MenuItemViewModel();

        public MenuService()
        {
            // Standard menus
            ViewModel.Items.AddRange(new IMenuItemViewModel[]
            {
                new FileMenuItemViewModel(),
                new WorkspaceMenuItemViewModel(),
                new WindowMenuItemViewModel(),
                new HelpMenuItemViewModel(),
            });
        }
    }
}