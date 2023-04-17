using System;
using System.Windows.Input;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Properties.Instrumentation
{
    public class ShaderViewModel : BasePropertyViewModel, IInstrumentableObject, IClosableObject
    {
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
        /// Instrumentation handler
        /// </summary>
        public InstrumentationState InstrumentationState
        {
            get => _instrumentationState;
            set
            {
                if (!this.CheckRaiseAndSetIfChanged(ref _instrumentationState, value))
                {
                    return;
                }
                
                // Get bus
                var bus = ConnectionViewModel?.GetSharedBus();
                if (bus == null)
                {
                    return;
                }

                // Submit request
                var request = bus.Add<SetShaderInstrumentationMessage>();
                request.featureBitSet = value.FeatureBitMask;
                request.shaderUID = Shader.GUID;
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public ShaderViewModel() : base("Shader", PropertyVisibility.WorkspaceTool)
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
            
            // Track
            _instrumentationState = new();
            
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
        /// Code listener
        /// </summary>
        private ShaderIdentifier _shader = new();

        /// <summary>
        /// Tracked state
        /// </summary>
        private InstrumentationState _instrumentationState = new();
    }
}