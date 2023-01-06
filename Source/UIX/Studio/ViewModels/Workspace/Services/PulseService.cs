using System;
using System.Net.NetworkInformation;
using Avalonia;
using Avalonia.Threading;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Logging;
using Studio.Services;

namespace Studio.ViewModels.Workspace.Listeners
{
    public class PulseService : ReactiveObject, IPulseService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Last known pulse time
        /// </summary>
        public DateTime LastPulseTime
        {
            get => _lastPulseTime; 
            set => this.RaiseAndSetIfChanged(ref _lastPulseTime, value);
        }
        
        /// <summary>
        /// Has the pulse been missed for an unreasonable amount of time?
        /// </summary>
        public bool MissedPulse
        {
            get => _missedPulse; 
            set => this.RaiseAndSetIfChanged(ref _missedPulse, value);
        }

        /// <summary>
        /// Target connection
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _connectionViewModel, value);

                OnConnectionChanged();
            }
        }

        public PulseService()
        {
            // Create timer on main thread
            _timer = new DispatcherTimer(DispatcherPriority.Background)
            {
                Interval = TimeSpan.FromSeconds(1),
                IsEnabled = true
            };

            // Subscribe tick
            _timer.Tick += OnTick;
            
            // Must call start manually
            _timer.Start();
        }

        private void OnTick(object? sender, EventArgs e)
        {
            // Check last known time, report lost if needed
            UpdateMissedPulse();
            
            // Valid connection?
            if (_connectionViewModel == null)
            {
                return;
            }

            // Create stream
            var stream = new StaticMessageView<PingPongMessage, ReadWriteMessageStream>(new ReadWriteMessageStream());

            // Add local ping
            var message = stream.Add<PingPongMessage>();
            message.timeStamp = EncodeDateTime(DateTime.Now);
            
            // Submit and wait for pong
            _connectionViewModel.Bridge?.GetOutput().AddStream(stream.Storage);
        }

        public void UpdateMissedPulse()
        {
            // Misssed if more than three seconds has passed
            bool missedPulse = (DateTime.Now - _lastPulseTime).Seconds > 3;

            // Changed?
            if (missedPulse != MissedPulse)
            {
                var logger = App.Locator.GetService<ILoggingService>();
                
                // Log event
                if (missedPulse)
                {
                    logger?.ViewModel.Events.Add(new LogEvent()
                    {
                        Severity = LogSeverity.Warning,
                        Message = $"Lost connection to {ConnectionViewModel?.Application?.DecoratedName ?? "Unknown"}"
                    });
                }
                else
                {
                    logger?.ViewModel.Events.Add(new LogEvent()
                    {
                        Severity = LogSeverity.Info,
                        Message = $"Regained connection to {ConnectionViewModel?.Application?.DecoratedName ?? "Unknown"}"
                    });
                }

                // OK
                MissedPulse = missedPulse;
            }
        }
        
        /// <summary>
        /// Encode the date time
        /// </summary>
        /// <param name="time">base date time</param>
        /// <returns></returns>
        private unsafe ulong EncodeDateTime(DateTime time)
        {
            long value = time.ToBinary();
            return *(ulong*)&value;
        }

        /// <summary>
        /// Decode a date time
        /// </summary>
        /// <param name="data">encoded date time</param>
        /// <returns></returns>
        private unsafe DateTime DecodeDateTime(ulong data)
        {
            return DateTime.FromBinary(*(long*)&data);
        }

        /// <summary>
        /// Invoked on connection changes
        /// </summary>
        private void OnConnectionChanged()
        {
            _connectionViewModel?.Bridge?.Register(PingPongMessage.ID, this);
        }
        
        /// <summary>
        /// Invoked on message streams
        /// </summary>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            foreach (PingPongMessage message in new StaticMessageView<PingPongMessage>(streams))
            {
                DateTime time = DecodeDateTime(message.timeStamp);

                Dispatcher.UIThread.InvokeAsync(() =>
                {
                    if (time > LastPulseTime)
                    {
                        LastPulseTime = time;
                        UpdateMissedPulse();
                    } 
                });
            }
        }

        /// <summary>
        /// Internal connection state
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Internal pulse time
        /// </summary>
        private DateTime _lastPulseTime = DateTime.Now;
        
        /// <summary>
        /// Pooling timer
        /// </summary>
        private DispatcherTimer _timer;

        /// <summary>
        /// Internal miss state
        /// </summary>
        private bool _missedPulse = false;
    }
}