using System.Collections.ObjectModel;
using Studio.ViewModels.Status;

namespace Studio.Services
{
    public interface IStatusService
    {
        /// <summary>
        /// All status view models
        /// </summary>
        public ObservableCollection<IStatusViewModel> ViewModels { get; }
    }
}