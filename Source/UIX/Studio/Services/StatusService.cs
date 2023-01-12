using System.Collections.ObjectModel;
using DynamicData;
using DynamicData.Binding;
using Studio.ViewModels.Status;

namespace Studio.Services
{
    public class StatusService : IStatusService
    {
        /// <summary>
        /// All status view models
        /// </summary>
        public ObservableCollection<IStatusViewModel> ViewModels { get; } = new();

        public StatusService()
        {
            // Standard objects
            ViewModels.AddRange(new IStatusViewModel[]
            {
                new LogStatusViewModel(),
                new NetworkStatusViewModel(),
                new InstrumentationStatusViewModel()
            });
        }
    }
}