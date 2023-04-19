using System;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Avalonia.Threading;
using Discovery.CLR;
using ReactiveUI;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Controls
{
    public class BusModeButtonViewModel : ReactiveObject
    {
        /// <summary>
        /// Current status color
        /// </summary>
        public IBrush? StatusColor
        {
            get => _statusColor;
            set => this.RaiseAndSetIfChanged(ref _statusColor, value);
        }

        /// <summary>
        /// Current geometry
        /// </summary>
        public StreamGeometry? StatusGeometry
        {
            get => _statusGeometry;
            set => this.RaiseAndSetIfChanged(ref _statusGeometry, value);
        }

        /// <summary>
        /// Toggle command
        /// </summary>
        public ICommand Toggle { get; }

        /// <summary>
        /// Constructor
        /// </summary>
        public BusModeButtonViewModel()
        {
            // Create commands
            Toggle = ReactiveCommand.Create(OnToggle);

            // Bind to current workspace
            App.Locator.GetService<IWorkspaceService>()?
                .WhenAnyValue(x => x.SelectedWorkspace)
                .Subscribe(x =>
            {
                // Invalid?
                if (x == null)
                {
                    StatusColor = new SolidColorBrush(ResourceLocator.GetResource<Color>("SystemAccentColor"));
                    StatusGeometry = ResourceLocator.GetIcon("Stop");
                    _currentService = null;
                    return;
                }

                // Fetch service
                _currentService = x.PropertyCollection.GetService<IBusPropertyService>();

                // Bind mode
                _currentService?
                    .WhenAnyValue(y => y.Mode)
                    .Subscribe(y =>
                    {
                        switch (y)
                        {
                            case BusMode.Immediate:
                                StatusColor = new SolidColorBrush(ResourceLocator.GetResource<Color>("SuccessColor"));
                                StatusGeometry = ResourceLocator.GetIcon("Play");
                                break;
                            case BusMode.RecordAndCommit:
                                StatusColor = new SolidColorBrush(ResourceLocator.GetResource<Color>("WarningColor"));
                                StatusGeometry = ResourceLocator.GetIcon("Pause");
                                break;
                            default:
                                throw new ArgumentOutOfRangeException(nameof(y), y, null);
                        }
                    });
            });
        }

        /// <summary>
        /// Invoked on instance toggles
        /// </summary>
        private void OnToggle()
        {
            if (_currentService == null)
            {
                return;
            }

            // Change mode
            switch (_currentService.Mode)
            {
                case BusMode.Immediate:
                    _currentService.Mode = BusMode.RecordAndCommit;
                    break;
                case BusMode.RecordAndCommit:
                    _currentService.Commit();
                    _currentService.Mode = BusMode.Immediate;
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
        }

        /// <summary>
        /// Current service model
        /// </summary>
        private IBusPropertyService? _currentService = null;

        /// <summary>
        /// Internal status color
        /// </summary>
        private IBrush? _statusColor;

        /// <summary>
        /// Internal geometry
        /// </summary>
        private StreamGeometry? _statusGeometry;
    }
}