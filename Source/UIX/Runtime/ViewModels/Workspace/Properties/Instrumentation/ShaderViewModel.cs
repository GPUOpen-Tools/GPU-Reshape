using System.Windows.Input;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Properties.Instrumentation
{
    public class ShaderViewModel : ReactiveObject, IPropertyViewModel, IInstrumentableObject, IClosableObject
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
        public PropertyVisibility Visibility => PropertyVisibility.WorkspaceTool;

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
        /// Invoked on closes
        /// </summary>
        public ICommand? CloseCommand { get; set; }

        /// <summary>
        /// Current shader
        /// </summary>
        public ShaderIdentifier Shader
        {
            get => _shader;
            set
            {
                this.RaiseAndSetIfChanged(ref _shader, value);
                OnShaderChanged();
            }
        }

        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set => this.RaiseAndSetIfChanged(ref _connectionViewModel, value);
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public ShaderViewModel()
        {
            CloseCommand = ReactiveCommand.Create(OnClose);
        }

        /// <summary>
        /// Invoked on close
        /// </summary>
        private void OnClose()
        {
            // Get bus
            var bus = ConnectionViewModel?.GetSharedBus();
            if (bus == null)
            {
                return;
            }

            // Submit request
            var request = bus.Add<SetShaderInstrumentationMessage>();
            request.featureBitSet = 0x0;
            request.shaderUID = Shader.GUID;
            
            // Remove from parent
            this.DetachFromParent();
        }

        /// <summary>
        /// Invoked on shader changes
        /// </summary>
        private void OnShaderChanged()
        {
            Name = Shader.Descriptor;
        }

        /// <summary>
        /// Get the owning workspace
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel? GetWorkspace()
        {
            return Parent?.GetRoot();
        }

        /// <summary>
        /// Invoked on instrumentation
        /// </summary>
        /// <param name="state"></param>
        public void SetInstrumentation(InstrumentationState state)
        {
            // Get bus
            var bus = ConnectionViewModel?.GetSharedBus();
            if (bus == null)
            {
                return;
            }

            // Submit request
            var request = bus.Add<SetShaderInstrumentationMessage>();
            request.featureBitSet = state.FeatureBitMask;
            request.shaderUID = Shader.GUID;
        }

        /// <summary>
        /// Code listener
        /// </summary>
        private ShaderIdentifier _shader = new();

        /// <summary>
        /// Internal name
        /// </summary>
        private string _name = "Shader";
        
        /// <summary>
        /// Internal connection
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}