using System.Collections.ObjectModel;
using Studio.ViewModels.Contexts;

namespace Studio.Services
{
    public interface IContextMenuService
    {
        /// <summary>
        /// Target view model of the context
        /// </summary>
        public object? TargetViewModel { get; set; }
        
        /// <summary>
        /// Root view model
        /// </summary>
        public IContextMenuItemViewModel ViewModel { get; }
    }
}