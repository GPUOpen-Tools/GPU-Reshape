using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia;
using DynamicData;
using ReactiveUI;
using Studio.Services;

namespace Studio.ViewModels.Menu
{
    public class FileMenuItemViewModel : ReactiveObject, IMenuItemViewModel
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
        public ObservableCollection<IMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Target command
        /// </summary>
        public ICommand? Command { get; } = null;

        /// <summary>
        /// Constructor
        /// </summary>
        public FileMenuItemViewModel()
        {
            Items.AddRange(new IMenuItemViewModel[]
            {
                new MenuItemViewModel()
                {
                    Header = "Settings",
                    Command = ReactiveCommand.Create(OnSettings)
                },
                
                new MenuItemViewModel()
                {
                    Header = "-"
                },
                
                new MenuItemViewModel()
                {
                    Header = "Exit",
                    Command = ReactiveCommand.Create(OnExit)
                }
            });
        }

        /// <summary>
        /// Invoked on settings
        /// </summary>
        private void OnSettings()
        {
            App.Locator.GetService<IWindowService>()?.OpenFor(new SettingsViewModel());
        }

        /// <summary>
        /// Invoked on exit
        /// </summary>
        private void OnExit()
        {
            App.Locator.GetService<IWindowService>()?.Exit();
        }

        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = "File";

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = true;
    }
}