using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Reactive.Linq;
using DynamicData;
using DynamicData.Binding;

namespace Studio.Services
{
    public class WorkspaceService : IWorkspaceService
    {
        /// <summary>
        /// Active workspaces
        /// </summary>
        public IObservableList<ViewModels.Workspace.WorkspaceConnection> Workspaces => _connections;

        public WorkspaceService()
        {
            
        }
        
        /// <summary>
        /// Add a new workspace
        /// </summary>
        /// <param name="connection">active connection</param>
        public void Add(ViewModels.Workspace.WorkspaceConnection connection)
        {
            _connections.Add(connection);
        }

        /// <summary>
        /// Remove an existing workspace
        /// </summary>
        /// <param name="connection">active connection</param>
        /// <returns>true if successfully removed</returns>
        public bool Remove(ViewModels.Workspace.WorkspaceConnection connection)
        {
            return _connections.Remove(connection);
        }

        /// <summary>
        /// Internal active workspaces
        /// </summary>
        private SourceList<ViewModels.Workspace.WorkspaceConnection> _connections = new();
        
    }
}