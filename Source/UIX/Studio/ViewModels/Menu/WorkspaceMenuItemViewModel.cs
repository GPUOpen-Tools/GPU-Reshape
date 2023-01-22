using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia;
using DynamicData;
using ReactiveUI;
using Studio.Services;

namespace Studio.ViewModels.Menu
{
    public class WorkspaceMenuItemViewModel : ReactiveObject, IMenuItemViewModel
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
        public WorkspaceMenuItemViewModel()
        {
            Items.AddRange(new IMenuItemViewModel[]
            {
                new MenuItemViewModel()
                {
                    Header = "Create Connection",
                    Command = ReactiveCommand.Create(OnCreateConnection)
                },
                
                new MenuItemViewModel()
                {
                    Header = "Close All Connections",
                    Command = ReactiveCommand.Create(OnCloseAllConnections)
                }
            });
        }

        /// <summary>
        /// Invoked on connection creation
        /// </summary>
        private void OnCreateConnection()
        {
            App.Locator.GetService<IWindowService>()?.OpenFor(new ConnectViewModel());
        }

        /// <summary>
        /// Invoked on close connections
        /// </summary>
        private void OnCloseAllConnections()
        {
            App.Locator.GetService<IWorkspaceService>()?.Clear();
        }

        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = "Workspace";

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = true;
    }
}