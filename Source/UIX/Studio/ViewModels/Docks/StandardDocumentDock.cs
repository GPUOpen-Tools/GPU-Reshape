using System;
using Dock.Model.Core;
using Studio.ViewModels.Documents;
using Dock.Model.ReactiveUI.Controls;
using ReactiveUI;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Docks
{
    public class StandardDocumentDock : DocumentDock
    {
        public StandardDocumentDock()
        {
            CreateDocument = ReactiveCommand.Create<object?>(OnCreateDocument);
        }

        private void OnCreateDocument(object? viewModel)
        {
            if (!CanCreateDocument)
            {
                return;
            }

            // Document index
            var index = VisibleDockables?.Count + 1;
            
            // Resulting document
            IDockable document;

            // TODO: Some sort of locator for this... need to figure out a system
            switch (viewModel)
            {
                default:
                    return;
                case WelcomeViewModel vm:
                    document = new WelcomeViewModel
                    {
                        Id = $"Document{index}",
                        Title = $"Document{index}"
                    };
                    break;
                case WorkspaceCollectionViewModel vm:
                    document = new WorkspaceOverviewViewModel
                    {
                        Id = $"WorkspaceOverview{index}", 
                        Title = $"{vm.ConnectionViewModel?.Application?.DecoratedName}",
                        Workspace = vm
                    };
                    break;
                case Documents.ShaderViewModel vm:
                    // TODO: This is getting REALLY ugly, next revision has to fix this nonsense
                    document = vm;
                    break;
            }
            
            Factory?.AddDockable(this, document);
            Factory?.SetActiveDockable(document);
            Factory?.SetFocusedDockable(this, document);
        }
    }
}
