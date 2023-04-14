using System.Windows.Input;
using Avalonia;
using Discovery.CLR;
using Message.CLR;
using ReactiveUI;
using Studio.Services;

namespace Studio.ViewModels
{
    public class LaunchViewModel : ReactiveObject
    {
        /// <summary>
        /// Connect to selected application
        /// </summary>
        public ICommand Start { get; }

        /// <summary>
        /// Current connection string
        /// </summary>
        public string ApplicationPath
        {
            get => _applicationPath;
            set
            {
                if (WorkingDirectoryPath == string.Empty || WorkingDirectoryPath == System.IO.Path.GetDirectoryName(_applicationPath))
                {
                    WorkingDirectoryPath = System.IO.Path.GetDirectoryName(value)!;
                }

                this.RaiseAndSetIfChanged(ref _applicationPath, value);
            }
        }

        /// <summary>
        /// Current connection string
        /// </summary>
        public string WorkingDirectoryPath
        {
            get => _workingDirectoryPath;
            set => this.RaiseAndSetIfChanged(ref _workingDirectoryPath, value);
        }

        /// <summary>
        /// Current connection string
        /// </summary>
        public string Arguments
        {
            get => _arguments;
            set => this.RaiseAndSetIfChanged(ref _arguments, value);
        }

        public LaunchViewModel()
        {
            // Create commands
            Start = ReactiveCommand.Create(OnStart);
        }

        /// <summary>
        /// Invoked on start
        /// </summary>
        private void OnStart()
        {
            if (App.Locator.GetService<IBackendDiscoveryService>()?.Service is not { } service)
            {
                return;
            }

            // Setup process info
            var processInfo = new DiscoveryProcessInfo();
            processInfo.applicationPath = _applicationPath;
            processInfo.workingDirectoryPath = _workingDirectoryPath;
            processInfo.arguments = _arguments;

            // Create environment view
            var view = new OrderedMessageView<ReadWriteMessageStream>(new ReadWriteMessageStream());

            // Dummy settings
            var configMsg = view.Add<SetInstrumentationConfigMessage>();
            configMsg.synchronousRecording = 1;

            // Dummy settings
            var instrumentMsg = view.Add<SetGlobalInstrumentationMessage>();
            instrumentMsg.featureBitSet = 0x1;

            // Start process
            service.StartBootstrappedProcess(processInfo, view.Storage);
        }

        /// <summary>
        /// Internal application path
        /// </summary>
        private string _applicationPath = string.Empty;

        /// <summary>
        /// Internal working directory path
        /// </summary>
        private string _workingDirectoryPath = string.Empty;

        /// <summary>
        /// Internal arguments
        /// </summary>
        private string _arguments = string.Empty;
    }
}