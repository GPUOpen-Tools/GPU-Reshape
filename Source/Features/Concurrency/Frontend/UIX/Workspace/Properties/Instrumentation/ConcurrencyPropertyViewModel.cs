using System.Windows.Input;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Models.Workspace;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.ResourceBounds.UIX.Workspace.Properties.Instrumentation
{
    public class ConcurrencyPropertyViewModel : BasePropertyViewModel, IInstrumentationProperty, IClosableObject
    {
        /// <summary>
        /// Concurrency feature
        /// </summary>
        public FeatureInfo FeatureInfo { get; set; }
        
        /// <summary>
        /// Invoked on close requests
        /// </summary>
        public ICommand? CloseCommand { get; set; }
        
        /// <summary>
        /// Constructor
        /// </summary>
        public ConcurrencyPropertyViewModel() : base("Concurrency", PropertyVisibility.WorkspaceTool)
        {
            CloseCommand = ReactiveCommand.Create(OnClose);
        }

        /// <summary>
        /// Invoked on close
        /// </summary>
        private void OnClose()
        {
            this.DetachFromParent();
        }

        /// <summary>
        /// Commit all changes
        /// </summary>
        /// <param name="state"></param>
        public void Commit(InstrumentationState state)
        {
            state.FeatureBitMask |= FeatureInfo.FeatureBit;
        }
    }
}