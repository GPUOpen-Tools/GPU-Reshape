using System.Collections.ObjectModel;
using ReactiveUI;
using Studio.ViewModels.Setting;

namespace Studio.ViewModels.Setting
{
    public class SettingItemViewModel : ReactiveObject, ISettingItemViewModel
    {
        /// <summary>
        /// Given header
        /// </summary>
        public string Header
        {
            get => _header;
            set => this.RaiseAndSetIfChanged(ref _header, value);
        }

        /// <summary>
        /// Is enabled?
        /// </summary>
        public bool IsEnabled
        {
            get => _isEnabled;
            set => this.RaiseAndSetIfChanged(ref _isEnabled, value);
        }

        /// <summary>
        /// Items within
        /// </summary>
        public ObservableCollection<ISettingItemViewModel> Items { get; } = new();

        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = "Settings";

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = true;
    }
}