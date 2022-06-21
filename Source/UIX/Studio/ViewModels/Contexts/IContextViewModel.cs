using System.Collections.ObjectModel;
using System.Windows.Input;

namespace Studio.ViewModels.Contexts
{
    public interface IContextViewModel
    {
        /// <summary>
        /// Target view model of the context
        /// </summary>
        public object? TargetViewModel { get; set; }
        
        /// <summary>
        /// Items within this context
        /// </summary>
        public ObservableCollection<IContextMenuItem> Items { get; }
    }
}