using System;
using System.Collections.Generic;
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

namespace Studio.ViewModels
{
    public class DockFactory : Factory
    {
        public IDocumentDock? Documents => _documentDock;

        public DockFactory(object context)
        {
            _context = context;
        }

        public override IDocumentDock CreateDocumentDock() => new StandardDocumentDock();

        public override IRootDock CreateLayout()
        {
            var welcome = new WelcomeViewModel {Id = "WelcomeDocument", Title = "Welcome"};
            var workspace = new WorkspaceViewModel {Id = "WorkspaceTool", Title = "Workspace"};
            var connections = new ConnectionViewModel {Id = "ConnectionTool", Title = "Connections"};
            var log = new LogViewModel {Id = "LogTool", Title = "Log"};
            var properties = new PropertyViewModel {Id = "PropertiesTool", Title = "Properties"};

            var leftDock = new ProportionalDock
            {
                Proportion = 0.25,
                Orientation = Orientation.Vertical,
                ActiveDockable = null,
                IsCollapsable = false,
                VisibleDockables = CreateList<IDockable>
                (
                    new ToolDock
                    {
                        ActiveDockable = workspace,
                        VisibleDockables = CreateList<IDockable>(workspace, connections),
                        Alignment = Alignment.Left
                    }
                )
            };

            var rightDock = new ProportionalDock
            {
                Proportion = 0.25,
                Orientation = Orientation.Vertical,
                ActiveDockable = null,
                IsCollapsable = true,
                CanPin = true,
                IsActive = false,
                VisibleDockables = CreateList<IDockable>
                (
                    new ToolDock
                    {
                        ActiveDockable = properties,
                        VisibleDockables = CreateList<IDockable>(properties),
                        Alignment = Alignment.Right,
                        GripMode = GripMode.Visible
                    }
                )
            };

            var bottomDock = new ProportionalDock
            {
                Proportion = 0.25,
                Orientation = Orientation.Horizontal,
                ActiveDockable = null,
                VisibleDockables = CreateList<IDockable>
                (
                    new ToolDock
                    {
                        ActiveDockable = log,
                        VisibleDockables = CreateList<IDockable>(log),
                        Alignment = Alignment.Right,
                        GripMode = GripMode.Visible
                    }
                )
            };

            var documentDock = new StandardDocumentDock
            {
                IsCollapsable = false,
                ActiveDockable = welcome,
                VisibleDockables = CreateList<IDockable>(welcome),
                CanCreateDocument = true
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
        
        private readonly object _context;
        
        private IRootDock? _rootDock;
        
        private IDocumentDock? _documentDock;
    }
}
