using Dock.Model.Core;
using Studio.ViewModels.Documents;
using Dock.Model.ReactiveUI.Controls;
using ReactiveUI;

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
                case Workspace.WorkspaceViewModel vm:
                    document = new WorkspaceOverviewViewModel
                    {
                        Id = $"WorkspaceOverview{index}", 
                        Title = $"{vm.Connection?.Application?.Name}",
                        Workspace = vm
                    };
                    break;
            }
            
            Factory?.AddDockable(this, document);
            Factory?.SetActiveDockable(document);
            Factory?.SetFocusedDockable(this, document);
        }
    }
}
