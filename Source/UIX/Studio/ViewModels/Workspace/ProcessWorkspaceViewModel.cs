using System.Diagnostics;
using DynamicData;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Services;

namespace Studio.ViewModels.Workspace
{
    public class ProcessWorkspaceViewModel : ReactiveObject, IWorkspaceViewModel
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

        public ProcessWorkspaceViewModel(string reservedToken)
        {
            _reservedToken = reservedToken;
            
            // The properties reference the abstract view model, not the actual top type
            _properties.WorkspaceViewModel = this;
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

            // Add general device pooler
            _properties.Services.Add(new ChildDevicePoolingService(_reservedToken)
            {
                TargetViewModel = PropertyCollection,
                ConnectionViewModel = _connection
            });
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Destroy the connection
            _connection?.Destruct();

            // Close process tree if possible, owned by this workspace
            if (_connection?.Application?.Pid is { } processId)
            {
                try
                {
                    // TODO: Graceful exit?
                    using Process process = Process.GetProcessById((int)processId);
                    process.Kill(entireProcessTree: true);
                }
                catch
                {
                    Studio.Logging.Error($"Failed to close workspace '{_connection.Application.DecoratedName} process");
                }
            }
            
            // Remove from service
            ServiceRegistry.Get<IWorkspaceService>()?.Remove(this);
        }

        /// <summary>
        /// Internal connection state
        /// </summary>
        private IConnectionViewModel? _connection;

        /// <summary>
        /// Internal property state
        /// </summary>
        private ProcessCollectionViewModel _properties = new();

        /// <summary>
        /// The reserved token (guid) to listen for
        /// </summary>
        private string _reservedToken;
    }
}