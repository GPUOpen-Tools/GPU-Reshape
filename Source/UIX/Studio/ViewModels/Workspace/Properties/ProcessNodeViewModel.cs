using ReactiveUI;

namespace Studio.ViewModels.Workspace.Properties
{
    public class ProcessNodeViewModel : BasePropertyViewModel
    {
        /// <summary>
        /// Assigned process ID
        /// </summary>
        public long PId
        {
            get => _pid;
            set => this.RaiseAndSetIfChanged(ref _pid, value);
        }

        public ProcessNodeViewModel(string name) : base(name, PropertyVisibility.WorkspaceTool)
        {
            
        }

        /// <summary>
        /// Internal process ID
        /// </summary>
        private long _pid = 0;
    }
}