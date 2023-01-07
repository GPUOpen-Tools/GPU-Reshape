using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia;
using DynamicData;
using ReactiveUI;
using Studio.Extensions;
using Studio.Services;

namespace Studio.ViewModels.Menu
{
    public class WindowMenuItemViewModel : ReactiveObject, IMenuItemViewModel
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
        public ICommand Command { get; } = null;

        /// <summary>
        /// Constructor
        /// </summary>
        public WindowMenuItemViewModel()
        {
            Items.AddRange(new IMenuItemViewModel[]
            {
                new MenuItemViewModel()
                {
                    Header = "Layout",
                    Items =
                    {
                        new MenuItemViewModel()
                        {
                            Header = "Restore Default",
                            Command = Reactive.Future(() => _windowService?.LayoutViewModel?.ResetLayout),
                        },
                        new MenuItemViewModel()
                        {
                            Header = "Close Default",
                            Command = Reactive.Future(() => _windowService?.LayoutViewModel?.CloseLayout),
                        }
                    }
                }
            });
        }

        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = "Window";

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = true;

        private IWindowService? _windowService = App.Locator.GetService<IWindowService>();
    }
}