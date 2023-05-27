using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Avalonia.Threading;
using Bridge.CLR;
using LiveChartsCore;
using LiveChartsCore.Drawing;
using LiveChartsCore.SkiaSharpView;
using LiveChartsCore.SkiaSharpView.Painting;
using Message.CLR;
using Microsoft.CodeAnalysis.VisualBasic.Syntax;
using ReactiveUI;
using SkiaSharp;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Controls
{
    public class DiagnosticPlotViewModel : ReactiveObject, IBridgeListener
    {
        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public IWorkspaceViewModel? Workspace
        {
            get => _workspaceViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _workspaceViewModel, value);
                OnWorkspaceChanged();
            }
        }

        /// <summary>
        /// All series
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
        private LiveChartsCore.Measure.Margin DrawMargin { get; set; } = new(55, 7.5f);

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
                MaxLimit = 175
            }
        };

        /// <summary>
        /// Get y axes
        /// </summary>
        public Axis[] YAxes { get; } =
        {
            new()
            {
                IsVisible = true,
                Name = "Frame (ms)",
                NameTextSize = 12,
                MinLimit = 0
            },

            new()
            {
                IsVisible = false,
                Padding = new Padding(0, 0, 0, 0),
                MinLimit = 0,
                MaxLimit = 50
            }
        };

        /// <summary>
        /// Constructor
        /// </summary>
        public DiagnosticPlotViewModel()
        {
            // Create present series
            _presentIntervalSeries = new()
            {
                Values = _presentIntervalValues,
                Fill = new SolidColorPaint(new SKColor(38, 45, 48)),
                Stroke = new SolidColorPaint(new SKColor(96, 125, 139)),
                GeometryFill = null,
                GeometryStroke = null,
                ScalesYAt = 0
            };

            // Create job series
            _jobSeries = new()
            {
                Values = _jobSeriesValues,
                Fill = new SolidColorPaint(new SKColor(31, 36, 56)),
                Stroke = new SolidColorPaint(new SKColor(63, 81, 181)),
                GeometryFill = null,
                GeometryStroke = null,
                ScalesYAt = 1
            };

            // All series
            Series = new ISeries[]
            {
                _presentIntervalSeries,
                _jobSeries
            };
        }

        /// <summary>
        /// Invoked when the workspace has changed
        /// </summary>
        public void OnWorkspaceChanged()
        {
            Workspace?.Connection?.Bridge?.Register(this);
        }

        /// <summary>
        /// Handle all incoming streams
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.Schema.IsOrdered())
            {
                return;
            }

            // Anything producing a step?
            bool requiresStep = false;

            // Default values
            double presentIntervalMS = _presentIntervalSeries.Values.LastOrDefault();
            double consumedJobs      = -1;

            // Visit all messages
            foreach (OrderedMessage message in new OrderedMessageView(streams))
            {
                switch (message.ID)
                {
                    case JobDiagnosticMessage.ID:
                    {
                        var msg = message.Get<JobDiagnosticMessage>();

                        // Consumed jobs?
                        if (_lastJobCount > msg.remaining)
                        {
                            consumedJobs = _lastJobCount - msg.remaining;
                        }

                        // Mark as step
                        _lastJobCount = msg.remaining;
                        requiresStep = true;
                        break;
                    }
                    case PresentDiagnosticMessage.ID:
                    {
                        var msg = message.Get<PresentDiagnosticMessage>();
                        
                        // Mark as step
                        presentIntervalMS = msg.intervalMS;
                        requiresStep = true;
                        break;
                    }
                }
            }

            // Any step?
            if (!requiresStep)
            {
                return;
            }

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                // Add series
                _presentIntervalValues.Add(presentIntervalMS);
                _jobSeriesValues.Add(consumedJobs);

                // Reduce present?
                if (_presentIntervalValues.Count > _maxFrameCount)
                {
                    _presentIntervalValues.RemoveAt(0);
                }

                // Reduce job?
                if (_jobSeriesValues.Count > _maxFrameCount)
                {
                    _jobSeriesValues.RemoveAt(0);
                }

                // Update limits
                YAxes[0].MaxLimit = _presentIntervalValues.Max() * 1.25f;
            });
        }

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IWorkspaceViewModel? _workspaceViewModel;

        /// <summary>
        /// All present values
        /// </summary>
        private ObservableCollection<double> _presentIntervalValues = new();

        /// <summary>
        /// All job values
        /// </summary>
        private ObservableCollection<double> _jobSeriesValues = new();

        /// <summary>
        /// Present series
        /// </summary>
        private LineSeries<double> _presentIntervalSeries;

        /// <summary>
        /// Job series
        /// </summary>
        private StepLineSeries<double> _jobSeries;

        /// <summary>
        /// Number of frames to keep track of
        /// </summary>
        private readonly uint _maxFrameCount = 175;

        /// <summary>
        /// Last tracked job count
        /// </summary>
        private uint _lastJobCount = 0;
    }
}