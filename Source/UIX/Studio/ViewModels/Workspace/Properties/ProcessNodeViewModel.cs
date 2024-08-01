using System.Reactive;
using System.Windows.Input;
using ReactiveUI;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Properties
{
    public class ProcessNodeViewModel : BasePropertyViewModel, IClosableObject
    {
        /// <summary>
        /// Assigned process ID
        /// </summary>
        public long PId
        {
            get => _pid;
            set => this.RaiseAndSetIfChanged(ref _pid, value);
        }
        
        /// <summary>
        /// Invoked on closes
        /// </summary>
        public ICommand? CloseCommand { get; set; }

        public ProcessNodeViewModel(string name) : base(name, PropertyVisibility.WorkspaceTool)
        {
            CloseCommand = ReactiveCommand.Create(OnClose);
        }

        /// <summary>
        /// Invoked on closes
        /// </summary>
        private void OnClose()
        {
            foreach (IPropertyViewModel propertyViewModel in Properties.Items)
            {
                if (propertyViewModel is IClosableObject closableObject)
                {
                    closableObject.CloseCommand?.Execute(Unit.Default);
                }
            }
            
            this.DetachFromParent(false);
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public override void Destruct()
        {
            foreach (IPropertyViewModel propertyViewModel in Properties.Items)
            {
                propertyViewModel.Destruct();
            }
        }

        /// <summary>
        /// Internal process ID
        /// </summary>
        private long _pid = 0;
    }
}