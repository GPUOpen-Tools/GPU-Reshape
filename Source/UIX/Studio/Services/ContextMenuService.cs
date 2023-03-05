using System.Collections.ObjectModel;
using DynamicData;
using Studio.ViewModels.Contexts;

namespace Studio.Services
{
    public class ContextMenuService : IContextMenuService
    {
        /// <summary>
        /// Target view model
        /// </summary>
        public object? TargetViewModel { get; set; }

        /// <summary>
        /// Root context menu view model
        /// </summary>
        public IContextMenuItemViewModel ViewModel { get; } = new ContextMenuItemViewModel();

        public ContextMenuService()
        {
            // Standard context menus
            ViewModel.Items.AddRange(new IContextMenuItemViewModel[]
            {
                new InstrumentContextViewModel(),
                new InstrumentPipelineFilterContextViewModel(),
                new CloseContextViewModel()
            });
        }
    }
}