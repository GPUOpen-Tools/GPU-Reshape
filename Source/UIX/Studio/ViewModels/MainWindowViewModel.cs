// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

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
        /// Current document layout
        /// </summary>
        public IDocumentLayoutViewModel? DocumentLayout => _factory?.Documents;

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

        /// <summary>
        /// Current workspace name
        /// </summary>
        public string WorkspaceLabel
        {
            get => _workspaceLabel;
            set => this.RaiseAndSetIfChanged(ref _workspaceLabel, value);
        }

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
            
            // Create commands
            ResetLayout = ReactiveCommand.Create(OnResetLayout);
            CloseLayout = ReactiveCommand.Create(OnCloseLayout);

            // Bind filters
            Status.WhenAnyValue(x => x).Subscribe(_ =>
            {
                this.RaisePropertyChanged(nameof(StatusLeft));
                this.RaisePropertyChanged(nameof(StatusRight));
            });
            
            // Bind workspace
            if (App.Locator.GetService<IWorkspaceService>() is { } workspaceService)
            {
                workspaceService.WhenAnyValue(x => x.SelectedWorkspace).Subscribe(x =>
                {
                    WorkspaceLabel = x?.Connection?.Application?.DecoratedName ?? "";
                });
            }
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

        /// <summary>
        /// Internal workspace name
        /// </summary>
        private string _workspaceLabel;
    }
}
