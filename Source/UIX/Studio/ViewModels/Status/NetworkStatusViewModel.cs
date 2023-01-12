using System;
using Avalonia;
using Avalonia.Threading;
using Bridge.CLR;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Status
{
    public class NetworkStatusViewModel : ReactiveObject, IStatusViewModel
    {
        /// <summary>
        /// Standard orientation
        /// </summary>
        public StatusOrientation Orientation => StatusOrientation.Right;

        /// <summary>
        /// Read string
        /// </summary>
        public string ReadAmount
        {
            get => _readAmount;
            set => this.RaiseAndSetIfChanged(ref _readAmount, value);
        }

        /// <summary>
        /// Write string
        /// </summary>
        public string WrittenAmount
        {
            get => _writtenAmount;
            set => this.RaiseAndSetIfChanged(ref _writtenAmount, value);
        }

        /// <summary>
        /// Read icon opacity
        /// </summary>
        public double ReadOpacity
        {
            get => _readOpacity;
            set => this.RaiseAndSetIfChanged(ref _readOpacity, value);
        }

        /// <summary>
        /// Write icon opacity
        /// </summary>
        public double WriteOpacity
        {
            get => _writeOpacity;
            set => this.RaiseAndSetIfChanged(ref _writeOpacity, value);
        }

        /// <summary>
        /// Decorate a measure
        /// </summary>
        private string DecorateByteCount(double bytes)
        {
            // Small nudge beyond the prefix window
            const float edgeFactor = 1.25f;

            return bytes switch
            {
                < 1e3 * edgeFactor => $"{(uint)(bytes)} B/s",
                < 1e6 * edgeFactor => $"{(uint)(bytes / 1e3)} KB/s",
                < 1e9 * edgeFactor => $"{(uint)(bytes / 1e6)} MB/s",
                _ => $"{(uint)(bytes / 1e9)} GB/s"
            };
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public NetworkStatusViewModel()
        {
            // Bind diagnostics
            _networkDiagnosticService?.WhenAnyValue(x => x.BytesReadPerSecond, x => x.BytesWrittenPerSecond).Subscribe(x =>
            {
                // Set strings
                ReadAmount = DecorateByteCount(x.Item1);
                WrittenAmount = DecorateByteCount(x.Item2);
                
                // Update opacity
                ReadOpacity = Math.Min(1.0f, 0.25f + x.Item1 / 1e6);
                WriteOpacity = Math.Min(1.0f, 0.25f + x.Item2 / 1e6);
            });
        }
        
        /// <summary>
        /// Network service
        /// </summary>
        private NetworkDiagnosticService? _networkDiagnosticService = App.Locator.GetService<NetworkDiagnosticService>();

        /// <summary>
        /// Internal read amount
        /// </summary>
        private string _readAmount = "";
        
        /// <summary>
        /// Internal write amount
        /// </summary>
        private string _writtenAmount = "";

        /// <summary>
        /// Internal read opacity
        /// </summary>
        private double _readOpacity = 0.25f;
        
        /// <summary>
        /// Internal write opacity
        /// </summary>
        private double _writeOpacity = 0.25f;
    }
}