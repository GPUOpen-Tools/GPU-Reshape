using DynamicData;
using ReactiveUI;

namespace Studio.ViewModels.Workspace.Properties
{
    public class WorkspaceCollectionViewModel : ReactiveObject, IPropertyViewModel
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name
        {
            get => ConnectionViewModel?.Application?.Name ?? "Invalid";
        }

        /// <summary>
        /// Visibility of this property
        /// </summary>
        public PropertyVisibility Visibility => PropertyVisibility.WorkspaceTool;

        /// <summary>
        /// Child properties
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; set; } = new SourceList<IPropertyViewModel>();

        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _connectionViewModel, value);

                OnConnectionChanged();
            }
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            CreateProperties();
        }

        /// <summary>
        /// Create all properties
        /// </summary>
        private void CreateProperties()
        {
            Properties.Clear();

            if (_connectionViewModel == null)
            {
                return;
            }
            
            Properties.AddRange(new IPropertyViewModel[]
            {
                new ShaderCollectionViewModel()
                {
                    ConnectionViewModel = _connectionViewModel
                },

                new PipelineCollectionViewModel()
                {
                    ConnectionViewModel = _connectionViewModel
                }
            });
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}