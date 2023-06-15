using ReactiveUI;

namespace Studio.ViewModels.Workspace.Objects
{
    public class ResourceValidationInstance : ReactiveObject
    {
        /// <summary>
        /// Instance message
        /// </summary>
        public string Message
        {
            get => _message;
            set => this.RaiseAndSetIfChanged(ref _message, value);
        }

        /// <summary>
        /// Instance message
        /// </summary>
        public string MessageNoNotify
        {
            get => _message;
            set => _message = value;
        }

        /// <summary>
        /// Number of occurrences
        /// </summary>
        public ulong Count
        {
            get => _count;
            set => this.RaiseAndSetIfChanged(ref _count, value);
        }

        /// <summary>
        /// Number of occurrences
        /// </summary>
        public ulong CountNoNotify
        {
            get => _count;
            set => _count = value;
        }

        /// <summary>
        /// Internal count
        /// </summary>
        private ulong _count;

        /// <summary>
        /// Internal message
        /// </summary>
        private string _message = string.Empty;
    }
}