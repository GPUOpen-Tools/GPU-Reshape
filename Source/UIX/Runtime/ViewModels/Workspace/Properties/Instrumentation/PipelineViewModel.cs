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
    public class PipelineViewModel : ReactiveObject, IPropertyViewModel, IInstrumentableObject, IClosableObject
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
        public PipelineIdentifier Pipeline
        {
            get => _pipeline;
            set
            {
                this.RaiseAndSetIfChanged(ref _pipeline, value);
                OnPipelineChanged();
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
                var request = bus.Add<SetPipelineInstrumentationMessage>();
                request.featureBitSet = value.FeatureBitMask;
                request.pipelineUID = Pipeline.GUID;
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
        public PipelineViewModel()
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
            var request = bus.Add<SetPipelineInstrumentationMessage>();
            request.featureBitSet = 0x0;
            request.pipelineUID = Pipeline.GUID;
            
            // Track
            _instrumentationState = new();
            
            // Remove from parent
            this.DetachFromParent();
        }

        /// <summary>
        /// Invoked on pipeline changes
        /// </summary>
        private void OnPipelineChanged()
        {
            Name = Pipeline.Descriptor;
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
        private PipelineIdentifier _pipeline;

        /// <summary>
        /// Internal name
        /// </summary>
        private string _name = "Shader";
        
        /// <summary>
        /// Internal connection
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Tracked state
        /// </summary>
        private InstrumentationState _instrumentationState = new();
    }
}