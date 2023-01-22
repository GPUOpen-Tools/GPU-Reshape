using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Windows.Input;
using Avalonia;
using DynamicData;
using ReactiveUI;
using Studio.Services;

namespace Studio.ViewModels.Menu
{
    public class HelpMenuItemViewModel : ReactiveObject, IMenuItemViewModel
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
        public HelpMenuItemViewModel()
        {
            Items.AddRange(new IMenuItemViewModel[]
            {
                new MenuItemViewModel()
                {
                    Header = "Documentation",
                    Command = ReactiveCommand.Create(OnDocumentation)
                },
                
                new MenuItemViewModel()
                {
                    Header = "-"
                },
                
                new MenuItemViewModel()
                {
                    Header = "About",
                    Command = ReactiveCommand.Create(OnAbout)
                }
            });
        }

        /// <summary>
        /// Invoked on documentation
        /// </summary>
        private void OnDocumentation()
        {
            // TODO: This is most definitely not the right way, will probably use the built chm or similar instead?
            System.Diagnostics.Process.Start(new ProcessStartInfo
            {
                FileName = "https://github.com/GPUOpen-Tools/gpu-validation/blob/main/Documentation/UIX.md",
                UseShellExecute = true
            });
        }

        /// <summary>
        /// Invoked on about
        /// </summary>
        private void OnAbout()
        {
            App.Locator.GetService<IWindowService>()?.OpenFor(new AboutViewModel());
        }

        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = "Help";

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = true;
    }
}