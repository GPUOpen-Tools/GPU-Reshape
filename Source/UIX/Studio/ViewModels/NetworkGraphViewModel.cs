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
using System.Collections.ObjectModel;
using System.Linq;
using System.Reflection;
using Avalonia.Threading;
using LiveChartsCore;
using LiveChartsCore.Drawing;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.SkiaSharpView.Painting;
using ReactiveUI;
using SkiaSharp;
using Studio.Extensions;
using Studio.Services;

namespace Studio.ViewModels
{
    public class NetworkGraphViewModel : ReactiveObject
    {
        /// <summary>
        /// All chart series
        /// </summary>
        public ISeries[] Series { get; }

        /// <summary>
        /// Default margin frame
        /// </summary>
        public DrawMarginFrame DrawMarginFrame { get; } = new()
        {
            Fill = null,
            Stroke = null
        };

        /// <summary>
        /// Default margin
        /// </summary>
        public LiveChartsCore.Measure.Margin DrawMargin { get; set; } = new(135, 7.5f, 7.5f, 7.5f);

        /// <summary>
        /// All x axes
        /// </summary>
        public Axis[] XAxes { get; } =
        {
            new()
            {
                IsVisible = false,
                Padding = new Padding(0, 0, 0, 0),
                MinLimit = 0,
                MaxLimit = 120
            }
        };

        /// <summary>
        /// All y axes
        /// </summary>
        public Axis[] YAxes { get; } =
        {
            new()
            {
                IsVisible = true,
                Name = "Throughput",
                NameTextSize = 11,
                NamePaint = new SolidColorPaint(new SKColor(160, 160, 160)),
                LabelsPaint = new SolidColorPaint(new SKColor(160, 160, 160)),
                SeparatorsPaint = new SolidColorPaint(new SKColor(55, 55, 55)) { StrokeThickness = 1 },
                MinLimit = 0,
                Labeler = FormatBytes
            }
        };

        /// <summary>
        /// Constructor
        /// </summary>
        public NetworkGraphViewModel()
        {
            var readSeries = new LineSeries<double>
            {
                Name = "Download",
                Values = _readValues,
                Fill = new SolidColorPaint(new SKColor(28, 42, 52)),
                Stroke = new SolidColorPaint(new SKColor(64, 144, 179)) { StrokeThickness = 1.5f },
                GeometryFill = null,
                GeometryStroke = null
            };

            var writeSeries = new LineSeries<double>
            {
                Name = "Upload",
                Values = _writeValues,
                Fill = new SolidColorPaint(new SKColor(52, 36, 20)),
                Stroke = new SolidColorPaint(new SKColor(200, 130, 50)) { StrokeThickness = 1.5f },
                GeometryFill = null,
                GeometryStroke = null
            };

            Series = new ISeries[]
            {
                readSeries,
                writeSeries
            };

            _networkDiagnosticService?.TickSubject.Subscribe(_ => OnTick());
        }

        /// <summary>
        /// Invoked on a diagnostic tick
        /// </summary>
        private void OnTick()
        {
            double read = _networkDiagnosticService?.BytesReadPerSecond ?? 0;
            double write = _networkDiagnosticService?.BytesWrittenPerSecond ?? 0;

            // Invoke on main thread
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                _readValues.Add(read);
                _writeValues.Add(write);

                if (_readValues.Count > _maxSamples)
                {
                    _readValues.RemoveAt(0);
                }

                if (_writeValues.Count > _maxSamples)
                {
                    _writeValues.RemoveAt(0);
                }

                // Set max value
                double max = Math.Max(_readValues.Max(), _writeValues.Max());
                YAxes[0].MaxLimit = max * 1.25 + 1;

                // Workaround for internal bug with re-rendering without layout invalidations
                if (++_stepFieldUpdateCounter % 100 == 0)
                {
                    YAxes.ForEach(x => StepCountField?.SetValue(x, 0));
                    XAxes.ForEach(x => StepCountField?.SetValue(x, 0));
                    _stepFieldUpdateCounter = 0;
                }
            });
        }

        /// <summary>
        /// Format a byte count as a readable
        /// </summary>
        private static string FormatBytes(double bytes)
        {
            const float edgeFactor = 1.25f;
            return bytes switch
            {
                < 1e3 * edgeFactor => $"{(uint)bytes} B/s",
                < 1e6 * edgeFactor => $"{(uint)(bytes / 1e3)} KB/s",
                < 1e9 * edgeFactor => $"{(uint)(bytes / 1e6)} MB/s",
                _ => $"{(uint)(bytes / 1e9)} GB/s"
            };
        }

        /// <summary>
        /// All read sample values
        /// </summary>
        private readonly ObservableCollection<double> _readValues = new();

        /// <summary>
        /// All write sample values
        /// </summary>
        private readonly ObservableCollection<double> _writeValues = new();

        /// <summary>
        /// Maximum number of retained samples
        /// </summary>
        private readonly uint _maxSamples = 120;

        /// <summary>
        /// Step field update counter, see usage
        /// </summary>
        private int _stepFieldUpdateCounter;

        /// <summary>
        /// Network diagnostic service
        /// </summary>
        private NetworkDiagnosticService? _networkDiagnosticService = ServiceRegistry.Get<NetworkDiagnosticService>();

        /// <summary>
        /// Internal step count field, used to work around a LiveCharts re-rendering bug
        /// </summary>
        private static readonly FieldInfo? StepCountField = typeof(Axis).BaseType!.GetField("_stepCount", BindingFlags.NonPublic | BindingFlags.Instance);
    }
}
