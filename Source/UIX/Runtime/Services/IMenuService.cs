using System.Collections.ObjectModel;
using Studio.ViewModels.Contexts;
using Studio.ViewModels.Menu;

namespace Studio.Services
{
    public interface IMenuService
    {
        /// <summary>
        /// Root view model
        /// </summary>
        public IMenuItemViewModel ViewModel { get; }
    }
}