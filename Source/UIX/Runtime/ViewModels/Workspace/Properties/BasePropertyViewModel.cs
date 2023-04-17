using DynamicData;
using ReactiveUI;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace.Properties
{
    public abstract class BasePropertyViewModel : ReactiveObject, IPropertyViewModel
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name
        {
            get => _name;
            set => this.RaiseAndSetIfChanged(ref _name, value);
        }

        /// <summary>
        /// Visibility of this property
        /// </summary>
        public PropertyVisibility Visibility => _visibility;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="name">Given initial name</param>
        /// <param name="visibility">Final visibility</param>
        public BasePropertyViewModel(string name, PropertyVisibility visibility)
        {
            Name = name;
            _visibility = visibility;
        }

        /// <summary>
        /// Parent property
        /// </summary>
        public IPropertyViewModel? Parent { get; set; }

        /// <summary>
        /// Child properties
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; set; } = new SourceList<IPropertyViewModel>();

        /// <summary>
        /// All services
        /// </summary>
        public ISourceList<IPropertyService> Services { get; set; } = new SourceList<IPropertyService>();
        
        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set => this.RaiseAndSetIfChanged(ref _connectionViewModel, value);
        }
        
        /// <summary>
        /// Internal connection
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Internal name
        /// </summary>
        private string _name = string.Empty;

        /// <summary>
        /// Internal visibility
        /// </summary>
        private PropertyVisibility _visibility;
    }
}