using ReactiveUI;

namespace Studio.ViewModels.Controls
{
    public class WorkspaceTreeItemViewModel : ObservableTreeItem
    {
        /// <summary>
        /// Underlying connection of this item
        /// </summary>
        public Workspace.WorkspaceConnection Connection
        {
            get { return _connection; }
            set
            {
                Text = value.Application?.Name ?? "Invalid";
                this.RaiseAndSetIfChanged(ref _connection, value);
            }
        }

        /// <summary>
        /// Internal connection state
        /// </summary>
        private Workspace.WorkspaceConnection _connection;
    }
}