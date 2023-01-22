using System;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Listeners;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace
{
    public class WorkspaceViewModel : ReactiveObject, IWorkspaceViewModel
    {
        /// <summary>
        /// Base property collection
        /// </summary>
        public IPropertyViewModel PropertyCollection => _properties;

        /// <summary>
        /// Active connection
        /// </summary>
        public IConnectionViewModel? Connection
        {
            get => _connection;
            set
            {
                this.RaiseAndSetIfChanged(ref _connection, value);
                
                OnConnectionChanged();
            }
        }

        /// <summary>
        /// Invoked when the connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            CreateProperties();
        }

        /// <summary>
        /// Create all properties within
        /// </summary>
        private void CreateProperties()
        {
            // Set connection
            _properties.ConnectionViewModel = _connection;

            // Create descriptor
            _properties.Descriptor = new WorkspaceOverviewDescriptor()
            {
                Workspace = this
            };
        }

        /// <summary>
        /// Internal connection state
        /// </summary>
        private IConnectionViewModel? _connection;

        /// <summary>
        /// Internal property state
        /// </summary>
        private WorkspaceCollectionViewModel _properties = new();
    }
}