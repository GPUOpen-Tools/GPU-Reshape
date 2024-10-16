// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
using System.Reactive.Linq;
using System.Windows.Input;
using Studio.Models.Documents;
using Studio.Models.Tools;
using Studio.ViewModels.Docks;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Tools;
using Studio.ViewModels.Views;
using Dock.Avalonia.Controls;
using Dock.Model.Controls;
using Dock.Model.Core;
using Dock.Model.ReactiveUI;
using Dock.Model.ReactiveUI.Controls;
using ReactiveUI;
using Runtime.ViewModels;

namespace Studio.ViewModels
{
    public class DockFactory : Factory
    {
        /// <summary>
        /// Document layout
        /// </summary>
        public IDocumentLayoutViewModel? Documents => _documentDock;
        
        /// <summary>
        /// Invoked on tooling expansion toggling
        /// </summary>
        public ICommand HideDockable { get; }
        
        /// <summary>
        /// Invoked on tooling close
        /// </summary>
        public ICommand CloseDockable { get; }

        /// <summary>
        /// Constructor
        /// </summary>
        public DockFactory(object context)
        {
            _context = context;
            
            // Create commands
            HideDockable = ReactiveCommand.Create<IDockable>(OnHideDockable);
            CloseDockable = ReactiveCommand.Create<IDockable>(OnCloseDockable);
        }

        /// <summary>
        /// Invoked on tooling closes
        /// </summary>
        private void OnCloseDockable(IDockable obj)
        {
            base.CloseDockable(obj);
        }

        /// <summary>
        /// On expansion changes
        /// </summary>
        /// <param name="dockable"></param>
        private void OnHideDockable(IDockable dockable)
        {
            if (dockable.Owner is IToolDock toolDock)
            {
                // Mark as collapsed
                toolDock.IsExpanded = false;
                toolDock.ActiveDockable = null;
                
                // Subscribe to changes
                toolDock.WhenAnyValue(x => x.ActiveDockable).WhereNotNull().Subscribe(_ =>
                {
                    if (!toolDock.IsExpanded)
                    {
                        toolDock.IsExpanded = true;
                    }
                });
            }
        }

        public override IDocumentDock CreateDocumentDock() => new StandardDocumentDock();

        public override IRootDock CreateLayout()
        {
            var welcome = new WelcomeViewModel {Id = "WelcomeDocument", Title = "Welcome" };
            var workspace = new WorkspaceViewModel {Id = "WorkspaceTool", Title = "Workspaces", CanClose = false};
            var files = new FilesViewModel() {Id = "FilesTool", Title = "Files", CanClose = false};
            var log = new LogViewModel {Id = "LogTool", Title = "Log", CanClose = false};
            var properties = new PropertyViewModel {Id = "PropertiesTool", Title = "Properties", CanClose = false};
            var shaders = new ShaderTreeViewModel() {Id = "ShadersTool", Title = "Shaders", CanClose = false};
            var pipelines = new PipelineTreeViewModel() {Id = "PipelinesTool", Title = "Pipelines", CanClose = false};

            // ISSUE: https://github.com/GPUOpen-Tools/GPU-Reshape/issues/37
            welcome.CanFloat = false;
            
            var leftDock = new ProportionalDock
            {
                Proportion = 0.20,
                Orientation = Orientation.Vertical,
                ActiveDockable = null,
                IsCollapsable = false,
                VisibleDockables = CreateList<IDockable>
                (
                    new ToolDock
                    {
                        ActiveDockable = workspace,
                        IsExpanded = true,
                        VisibleDockables = CreateList<IDockable>(workspace, files),
                        Alignment = Alignment.Left
                    }
                )
            };

            var rightDock = new ProportionalDock
            {
                Proportion = 0.20,
                Orientation = Orientation.Vertical,
                ActiveDockable = null,
                IsCollapsable = true,
                CanPin = true,
                IsActive = false,
                VisibleDockables = CreateList<IDockable>
                (
                    new ToolDock
                    {
                        ActiveDockable = shaders,
                        IsExpanded = true,
                        VisibleDockables = CreateList<IDockable>(properties, shaders, pipelines),
                        Alignment = Alignment.Right,
                        GripMode = GripMode.Visible
                    }
                )
            };

            var bottomDock = new ProportionalDock
            {
                Proportion = 0.15,
                Orientation = Orientation.Horizontal,
                ActiveDockable = null,
                VisibleDockables = CreateList<IDockable>
                (
                    new ToolDock
                    {
                        ActiveDockable = log,
                        IsExpanded = true,
                        VisibleDockables = CreateList<IDockable>(log),
                        Alignment = Alignment.Bottom,
                        GripMode = GripMode.Visible
                    }
                )
            };

            var documentDock = new StandardDocumentDock
            {
                IsCollapsable = false,
                ActiveDockable = welcome,
                VisibleDockables = CreateList<IDockable>(welcome),
                CanCreateDocument = false
            };

            var centerLayout = new ProportionalDock
            {
                Orientation = Orientation.Horizontal,
                VisibleDockables = CreateList<IDockable>
                (
                    leftDock,
                    new ProportionalDockSplitter(),
                    documentDock,
                    new ProportionalDockSplitter(),
                    rightDock
                )
            };

            var mainLayout = new ProportionalDock
            {
                Orientation = Orientation.Vertical,
                VisibleDockables = CreateList<IDockable>
                (
                    centerLayout,
                    new ProportionalDockSplitter(),
                    bottomDock
                )
            };

            var dashboardView = new DashboardViewModel
            {
                Id = "Dashboard",
                Title = "Dashboard"
            };

            var homeView = new HomeViewModel
            {
                Id = "Home",
                Title = "Home",
                ActiveDockable = mainLayout,
                VisibleDockables = CreateList<IDockable>(mainLayout)
            };

            var rootDock = CreateRootDock();

            rootDock.IsCollapsable = false;
            rootDock.ActiveDockable = dashboardView;
            rootDock.DefaultDockable = homeView;
            rootDock.VisibleDockables = CreateList<IDockable>(dashboardView, homeView);

            _documentDock = documentDock;
            _rootDock = rootDock;
            
            return rootDock;
        }

        public override void InitLayout(IDockable layout)
        {
            ContextLocator = new Dictionary<string, Func<object>>
            {
                ["WelcomeDocument"] = () => new WelcomeDocument(),
                ["WorkspaceTool"] = () => new ViewModels.Tools.WorkspaceViewModel(),
                ["ConnectionTool"] = () => new Connection(),
                ["PropertyTool"] = () => new Property(),
                ["LogTool"] = () => new Log(),
                ["DashboardTool"] = () => layout,
                ["Home"] = () => _context
            };
            
            DockableLocator = new Dictionary<string, Func<IDockable?>>()
            {
                ["Root"] = () => _rootDock,
                ["Documents"] = () => _documentDock
            };

            HostWindowLocator = new Dictionary<string, Func<IHostWindow>>
            {
                [nameof(IDockWindow)] = () => new HostWindow()
            };

            base.InitLayout(layout);
        }
        
        /// <summary>
        /// Internal context
        /// </summary>
        private readonly object _context;
        
        /// <summary>
        /// Root dock
        /// </summary>
        private IRootDock? _rootDock;
        
        /// <summary>
        /// Central document dock
        /// </summary>
        private IDocumentLayoutViewModel? _documentDock;
    }
}
