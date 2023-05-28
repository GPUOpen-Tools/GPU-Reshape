using System.Reactive;
using System.Reactive.Subjects;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using ReactiveUI;
using Studio.Services;

namespace Studio.ViewModels.Documents
{
    public class WelcomeViewModel : Document, IDocumentViewModel
    {
        /// <summary>
        /// Descriptor setter, constructs the top document from given descriptor
        /// </summary>
        public IDescriptor? Descriptor
        {
            set
            {
                
            }
        }
        
        /// <summary>
        /// Create connection command
        /// </summary>
        public ICommand CreateConnection { get; }
        
        /// <summary>
        /// Create connection command
        /// </summary>
        public ICommand LaunchApplication { get; }
        
        /// <summary>
        /// Close command
        /// </summary>
        public ICommand Close { get; }

        /// <summary>
        /// Closed event
        /// </summary>
        public Subject<Unit> OnClose { get; } = new();
        
        /// <summary>
        /// Document icon
        /// </summary>
        public StreamGeometry? Icon
        {
            get => _icon;
            set => this.RaiseAndSetIfChanged(ref _icon, value);
        }

        /// <summary>
        /// Icon color
        /// </summary>
        public Color? IconForeground
        {
            get => _iconForeground;
            set => this.RaiseAndSetIfChanged(ref _iconForeground, value);
        }

        public WelcomeViewModel()
        {
            CreateConnection = ReactiveCommand.Create(OnCreateConnection);
            LaunchApplication = ReactiveCommand.Create(OnLaunchApplication);
            Close = ReactiveCommand.Create(OnCloseStudio);
        }

        /// <summary>
        /// Invoked on connection
        /// </summary>
        private void OnCreateConnection()
        {
            App.Locator.GetService<IWindowService>()?.OpenFor(new ConnectViewModel());
        }

        /// <summary>
        /// Invoked on launch
        /// </summary>
        private void OnLaunchApplication()
        {
            App.Locator.GetService<IWindowService>()?.OpenFor(new LaunchViewModel());
        }

        /// <summary>
        /// Invoked on close
        /// </summary>
        private void OnCloseStudio()
        {
            OnClose.OnNext(Unit.Default);
        }
        
        /// <summary>
        /// Internal icon state
        /// </summary>
        private StreamGeometry? _icon = ResourceLocator.GetIcon("App");

        /// <summary>
        /// Internal icon color
        /// </summary>
        private Color? _iconForeground = ResourceLocator.GetResource<Color>("SystemBaseHighColor");
    }
}