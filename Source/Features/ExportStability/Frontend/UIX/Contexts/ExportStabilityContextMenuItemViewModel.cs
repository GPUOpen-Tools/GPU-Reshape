using System.Collections.ObjectModel;
using System.Windows.Input;
using Message.CLR;
using ReactiveUI;
using Studio.ViewModels.Contexts;
using Studio.ViewModels.Workspace.Properties;

namespace Features.ResourceBounds.UIX.Contexts
{
    public class ExportStabilityContextMenuItemViewModel : ReactiveObject, IContextMenuItemViewModel
    {
        /// <summary>
        /// Target view model of the context
        /// </summary>
        public object? TargetViewModel
        {
            get => _targetViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _targetViewModel, value);
                IsEnabled = _targetViewModel is IPropertyViewModel;
            }
        }

        /// <summary>
        /// Display header of this context model
        /// </summary>
        public string Header { get; set; } = "Export Stability";
        
        /// <summary>
        /// All items within this context model
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Context command
        /// </summary>
        public ICommand Command { get; }

        /// <summary>
        /// Is this context enabled?
        /// </summary>
        public bool IsEnabled
        {
            get => _isEnabled;
            set => this.RaiseAndSetIfChanged(ref _isEnabled, value);
        }

        public ExportStabilityContextMenuItemViewModel()
        {
            Command = ReactiveCommand.Create(OnInvoked);
        }

        /// <summary>
        /// Command implementation
        /// </summary>
        private void OnInvoked()
        {
            if (_targetViewModel is not IPropertyViewModel property)
            {
                return;
            }

            Bridge.CLR.IBridge? bridge = property.ConnectionViewModel?.Bridge;
            if (bridge == null)
            {
                return;
            }

            // Allocate ordered
            var view = new OrderedMessageView<ReadWriteMessageStream>(new ReadWriteMessageStream());

            // Start all instrumentation features
            var instrument = view.Add<SetGlobalInstrumentationMessage>();
            instrument.featureBitSet = ~0ul;
            
            // Submit!
            bridge.GetOutput().AddStream(view.Storage);
            bridge.Commit();
        }

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = false;

        /// <summary>
        /// Internal target view model
        /// </summary>
        private object? _targetViewModel;
    }
}