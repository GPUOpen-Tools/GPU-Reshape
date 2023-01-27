using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Windows.Input;
using Avalonia;
using Discovery.CLR;
using Dock.Model.Controls;
using Dock.Model.Core;
using ReactiveUI;
using Runtime.ViewModels;
using Studio.Extensions;
using Studio.Services;
using Studio.ViewModels.Menu;
using Studio.ViewModels.Status;

namespace Studio.ViewModels
{
    public class MainWindowViewModel : ReactiveObject, ILayoutViewModel
    {
        /// <summary>
        /// Current layout hosted
        /// </summary>
        public IRootDock? Layout
        {
            get => _layout;
            set => this.RaiseAndSetIfChanged(ref _layout, value);
        }
        
        /// <summary>
        /// Open a document
        /// </summary>
        public ICommand? OpenDocument { get; set; }
        
        /// <summary>
        /// Reset the layout
        /// </summary>
        public ICommand ResetLayout { get; }
        
        /// <summary>
        /// Close the layout
        /// </summary>
        public ICommand CloseLayout { get; }

        /// <summary>
        /// Menu view models
        /// </summary>
        public ObservableCollection<IMenuItemViewModel>? Menu => AvaloniaLocator.Current.GetService<IMenuService>()?.ViewModel.Items;

        /// <summary>
        /// Status view models
        /// </summary>
        public ObservableCollection<IStatusViewModel>? Status => AvaloniaLocator.Current.GetService<IStatusService>()?.ViewModels;

        /// <summary>
        /// Filtered lhs status
        /// </summary>
        public IEnumerable<IStatusViewModel>? StatusLeft => Status?.Where(s => s.Orientation == StatusOrientation.Left);

        /// <summary>
        /// Filtered rhs status
        /// </summary>
        public IEnumerable<IStatusViewModel>? StatusRight => Status?.Where(s => s.Orientation == StatusOrientation.Right);

        public MainWindowViewModel()
        {
            // Attach to window service
            if (App.Locator.GetService<IWindowService>() is { } service)
            {
                service.LayoutViewModel = this;
            }
            
            // Create factory
            _factory = new DockFactory(new object());

            // Create base layout
            Layout = _factory.CreateLayout();
            if (Layout != null)
            {
                // Set layout
                _factory.InitLayout(Layout);
                
                // If root is valid, navigate
                if (Layout is { } root)
                {
                    root.Navigate.Execute("Home");
                }
            }
            
            // Attach document interactions, future them for layout invalidation
            OpenDocument = Reactive.Future(() => _factory.Documents?.CreateDocument);

            // Create commands
            ResetLayout = ReactiveCommand.Create(OnResetLayout);
            CloseLayout = ReactiveCommand.Create(OnCloseLayout);

            // Bind filters
            Status.WhenAnyValue(x => x).Subscribe(_ =>
            {
                this.RaisePropertyChanged(nameof(StatusLeft));
                this.RaisePropertyChanged(nameof(StatusRight));
            });
        }

        /// <summary>
        /// Invoked on layout close
        /// </summary>
        private void OnCloseLayout()
        {
            if (Layout is IDock dock)
            {
                if (dock.Close.CanExecute(null))
                {
                    dock.Close.Execute(null);
                }
            }

            // Get valid discovery
            if (App.Locator.GetService<IBackendDiscoveryService>() is { Service: { } } discovery)
            {
                // In case this is not a global install, remove the hooks on exit
                if (!discovery.Service.IsGloballyInstalled())
                {
                    discovery.Service.Stop();
                }
            }
        }

        /// <summary>
        /// Invoked on layout resets
        /// </summary>
        private void OnResetLayout()
        {
            // Valid layout?
            if (Layout != null)
            {
                if (Layout.Close.CanExecute(null))
                {
                    Layout.Close.Execute(null);
                }
            }

            // Attempt to create a new layout
            var layout = _factory?.CreateLayout();
            if (layout == null)
            {
                return;
            }
            
            // Initialize
            Layout = layout;
            _factory?.InitLayout(layout);
        }

        /// <summary>
        /// Internal factory state
        /// </summary>
        private readonly DockFactory? _factory;
        
        /// <summary>
        /// Internal layout state
        /// </summary>
        private IRootDock? _layout;
    }
}
