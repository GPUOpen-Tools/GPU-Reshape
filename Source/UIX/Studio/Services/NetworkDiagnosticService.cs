using System;
using Avalonia;
using Avalonia.Threading;
using Bridge.CLR;
using ReactiveUI;
using Studio.Extensions;

namespace Studio.Services
{
    public class NetworkDiagnosticService : ReactiveObject
    {
        /// <summary>
        /// Total number of bytes read per second
        /// </summary>
        public double BytesReadPerSecond
        {
            get => _bytesReadPerSecond;
            set => this.RaiseAndSetIfChanged(ref _bytesReadPerSecond, value);
        }
        
        /// <summary>
        /// Total number of bytes written per second
        /// </summary>
        public double BytesWrittenPerSecond
        {
            get => _bytesWrittenPerSecond;
            set => this.RaiseAndSetIfChanged(ref _bytesWrittenPerSecond, value);
        }
        
        public NetworkDiagnosticService()
        {
            // Create timer on main thread
            _timer = new DispatcherTimer(DispatcherPriority.Background)
            {
                Interval = TimeSpan.FromMilliseconds(1000),
                IsEnabled = true
            };

            // Subscribe tick
            _timer.Tick += OnTick;
            
            // Must call start manually (a little vague)
            _timer.Start();
        }

        private void OnTick(object? sender, EventArgs e)
        {
            // Summarized
            BridgeInfo info = new BridgeInfo();
            
            // Accumulate info
            App.Locator.GetService<IWorkspaceService>()?.Workspaces.Items.ForEach(x =>
            {
                BridgeInfo workspaceInfo = x.Connection?.Bridge?.GetInfo() ?? new BridgeInfo();
                info.bytesWritten += workspaceInfo.bytesWritten;
                info.bytesRead += workspaceInfo.bytesRead;
            });

            // Average
            BytesReadPerSecond = (info.bytesRead - _lastInfo.bytesRead) / _timer.Interval.TotalSeconds;
            BytesWrittenPerSecond = (info.bytesWritten - _lastInfo.bytesWritten) / _timer.Interval.TotalSeconds;

            // Set last
            _lastInfo = info;
        }

        /// <summary>
        /// Last info state
        /// </summary>
        private BridgeInfo _lastInfo = new BridgeInfo();

        /// <summary>
        /// Internal read speed
        /// </summary>
        private double _bytesReadPerSecond;
        
        /// <summary>
        /// Internal write speed
        /// </summary>
        private double _bytesWrittenPerSecond;

        /// <summary>
        /// Internal timer
        /// </summary>
        private readonly DispatcherTimer _timer;
    }
}