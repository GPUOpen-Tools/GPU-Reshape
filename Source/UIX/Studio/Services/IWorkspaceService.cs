using System;
using DynamicData;

namespace Studio.Services
{
    public interface IWorkspaceService
    {
        /// <summary>
        /// All active workspaces
        /// </summary>
        IObservableList<ViewModels.Workspace.WorkspaceConnection> Workspaces { get; }
        
        /// <summary>
        /// Add a new workspace
        /// </summary>
        /// <param name="connection">active connection</param>
        void Add(ViewModels.Workspace.WorkspaceConnection connection);

        /// <summary>
        /// Remove an existing workspace
        /// </summary>
        /// <param name="connection">active connection</param>
        /// <returns>true if successfully removed</returns>
        bool Remove(ViewModels.Workspace.WorkspaceConnection connection);
    }
}