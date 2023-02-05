using Studio.ViewModels.Setting;

namespace Studio.Services
{
    public interface ISettingsService
    {
        /// <summary>
        /// Root view model
        /// </summary>
        public ISettingItemViewModel ViewModel { get; }
    }
}