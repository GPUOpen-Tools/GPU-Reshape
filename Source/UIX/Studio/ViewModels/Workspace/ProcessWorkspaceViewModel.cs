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
            
            // Subscribe on collections behalf
            _properties.CloseCommand = ReactiveCommand.Create(OnClose);
        }

        /// <summary>
        /// Invoked on close
        /// </summary>
        private void OnClose()
        {
            // Remove from service
            App.Locator.GetService<IWorkspaceService>()?.Remove(this);
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
            // Close all child collections
            foreach (var collectionViewModel in _properties.GetProperties<WorkspaceCollectionViewModel>().ToArray())
            {
                if (collectionViewModel is IClosableObject closable)
                {
                    closable.CloseCommand?.Execute(Unit.Default);
                }
            }
            
            // Destroy all properties and services
            DestructPropertyTree(_properties);
            
            // Destroy the connection
            _connection?.Destruct();
        }

        /// <summary>
        /// Destruct a given property tree hierarchically
        /// </summary>
        private void DestructPropertyTree(IPropertyViewModel propertyViewModel)
        {
            // Children first
            // ... I forgot the name, depth-something-something, this is the opposite of job security.
            foreach (IPropertyViewModel child in propertyViewModel.Properties.Items)
            {
                DestructPropertyTree(child);
            }
            
            // Destroy associated services
            foreach (IPropertyService propertyService in propertyViewModel.Services.Items)
            {
                propertyService.Destruct();
            }

            // Finally destruct the property
            propertyViewModel.Destruct();
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