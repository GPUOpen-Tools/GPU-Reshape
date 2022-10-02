using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Studio.Models;
using Dock.Model.Controls;
using Dock.Model.Core;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Status;
using Studio.Views;

namespace Studio.ViewModels
{
    public class MainWindowViewModel : ReactiveObject
    {
        public IRootDock? Layout
        {
            get => _layout;
            set => this.RaiseAndSetIfChanged(ref _layout, value);
        }

        /// <summary>
        /// Reset the layout
        /// </summary>
        public ICommand ResetLayout { get; }

        /// <summary>
        /// Open the settings window
        /// </summary>
        public ICommand OpenSettings { get; }

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
            _factory = new DockFactory(new object());

            Layout = _factory?.CreateLayout();
            if (Layout is { })
            {
                _factory?.InitLayout(Layout);
                if (Layout is { } root)
                {
                    root.Navigate.Execute("Home");
                }
            }
            
            // Attach document interactions
            Interactions.DocumentInteractions.OpenDocument.InvokeCommand(_factory.Documents.CreateDocument);

            // Create commands
            ResetLayout = ReactiveCommand.Create(OnResetLayout);
            OpenSettings = ReactiveCommand.Create(OnOpenSettings);

            // Bind filters
            Status.WhenAnyValue(x => x).Subscribe(x =>
            {
                this.RaisePropertyChanged(nameof(StatusLeft));
                this.RaisePropertyChanged(nameof(StatusRight));
            });
        }

        public void CloseLayout()
        {
            if (Layout is IDock dock)
            {
                if (dock.Close.CanExecute(null))
                {
                    dock.Close.Execute(null);
                }
            }
        }

        /// <summary>
        /// Invoked on layout resets
        /// </summary>
        public void OnResetLayout()
        {
            if (Layout is not null)
            {
                if (Layout.Close.CanExecute(null))
                {
                    Layout.Close.Execute(null);
                }
            }

            var layout = _factory?.CreateLayout();
            if (layout is not null)
            {
                Layout = layout;
                _factory?.InitLayout(layout);
            }
        }

        /// <summary>
        /// Invoked on setting opening
        /// </summary>
        public void OnOpenSettings()
        {
            // Must have desktop lifetime
            if (Application.Current?.ApplicationLifetime is not IClassicDesktopStyleApplicationLifetime desktop)
            {
                return;
            }
            
            // Create dialog
            var dialog = new SettingsWindow()
            {
                WindowStartupLocation = WindowStartupLocation.CenterOwner
            };

            // Blocking
            dialog.ShowDialog(desktop.MainWindow);
        }

        private readonly DockFactory? _factory;
        
        private IRootDock? _layout;
    }
}
