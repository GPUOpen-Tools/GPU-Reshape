using System;
using System.Diagnostics;
using System.Windows.Input;
using Avalonia;
using Studio.Models;
using Dock.Model.Controls;
using Dock.Model.Core;
using DynamicData;
using ReactiveUI;
using Studio.Services;

namespace Studio.ViewModels
{
    public class MainWindowViewModel : ReactiveObject
    {
        public IRootDock? Layout
        {
            get => _layout;
            set => this.RaiseAndSetIfChanged(ref _layout, value);
        }

        public ICommand NewLayout { get; }

        public MainWindowViewModel()
        {
            _factory = new DockFactory(new DemoData());

            Layout = _factory?.CreateLayout();
            if (Layout is { })
            {
                _factory?.InitLayout(Layout);
                if (Layout is { } root)
                {
                    root.Navigate.Execute("Home");
                }
            }
            
            // Attach workspace creation
            App.Locator.GetService<IWorkspaceService>()?.Workspaces.Connect()
                .OnItemAdded(_factory.Documents.CreateDocument.Execute)
                .Subscribe();

            // Create commands
            NewLayout = ReactiveCommand.Create(ResetLayout);
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

        public void ResetLayout()
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

        private readonly DockFactory? _factory;
        
        private IRootDock? _layout;
    }
}
