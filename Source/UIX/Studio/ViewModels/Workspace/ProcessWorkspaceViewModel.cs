using System.Linq;
using System.Reactive;
using Avalonia;
using DynamicData;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Traits;
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