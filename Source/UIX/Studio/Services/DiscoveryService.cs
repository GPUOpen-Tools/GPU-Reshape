using DynamicData;
using Studio.ViewModels.Menu;
using Studio.ViewModels.Setting;

namespace Studio.Services
{
    public class SettingsService : ISettingsService
    {
        /// <summary>
        /// Root settings view model
        /// </summary>
        public ISettingItemViewModel ViewModel { get; } = new SettingItemViewModel();

        public SettingsService()
        {
            // Standard settings
            ViewModel.Items.AddRange(new ISettingItemViewModel[]
            {
                new DiscoverySettingItemViewModel()
            });
        }
    }
}