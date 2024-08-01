// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

using System;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Message.CLR;
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
            _workspaceService?
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
                            case BusMode.Discard:
                                StatusColor = new SolidColorBrush(ResourceLocator.GetResource<Color>("ErrorColor"));
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
                case BusMode.Discard:
                    _currentService.Mode = BusMode.Immediate;
                    break;
                default:
                    throw new ArgumentOutOfRangeException();
            }
            
            // Update selected workspace, if any
            if (_workspaceService?.SelectedWorkspace is { Connection: { } connection })
            {
                var range = connection.GetSharedBus().Add<PauseInstrumentationMessage>();
                range.paused = _currentService.Mode != BusMode.Immediate ? 1 : 0;
            }
        }

        /// <summary>
        /// Current service model
        /// </summary>
        private IBusPropertyService? _currentService = null;

        /// <summary>
        /// Shared workspace service
        /// </summary>
        private IWorkspaceService? _workspaceService = App.Locator.GetService<IWorkspaceService>();

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