using System;
using System.Diagnostics;
using System.Reactive;
using System.Reactive.Subjects;
using Avalonia;
using Avalonia.Controls.Notifications;
using Avalonia.Platform;
using Avalonia.Threading;
using Bridge.CLR;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Workspace;

namespace Studio.ViewModels.Workspace
{
    public class ConnectionViewModel : ReactiveObject, IConnectionViewModel
    {
        /// <summary>
        /// Invoked during connection
        /// </summary>
        public ISubject<Unit> Connected { get; }
        
        /// <summary>
        /// Invoked during connection rejection
        /// </summary>
        public ISubject<Unit> Refused { get; }

        /// <summary>
        /// Local time on the remote endpoint
        /// </summary>
        public DateTime LocalTime
        {
            get => localTime;
            set => this.RaiseAndSetIfChanged(ref localTime, value);
        }

        /// <summary>
        /// Underlying bridge of this connection
        /// </summary>
        public IBridge? Bridge
        {
            get
            {
                lock (this)
                {
                    return _remote;
                }
            }
        }

        /// <summary>
        /// Is the connection alive?
        /// </summary>
        public bool IsConnected => _remote != null;

        /// <summary>
        /// Endpoint application info
        /// </summary>
        public Models.Workspace.ApplicationInfo? Application { get; private set; }

        public ConnectionViewModel()
        {
            // Create subjects
            Connected = new Subject<Unit>();
            Refused = new Subject<Unit>();
            
            // Create dispatcher local to the connection
            _dispatcher = new Dispatcher(AvaloniaLocator.Current.GetService<IPlatformThreadingInterface>());
            
            // Create timer on main thread
            _timer = new DispatcherTimer(DispatcherPriority.Background)
            {
                Interval = TimeSpan.FromMilliseconds(15),
                IsEnabled = true
            };

            // Subscribe tick
            _timer.Tick += OnTick;
            
            // Must call start manually (a little vague)
            _timer.Start();
        }

        /// <summary>
        /// Invoked during timer ticks
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnTick(object? sender, EventArgs e)
        {
            // Commit all changes
            Commit();
        }

        /// <summary>
        /// Connect synchronously
        /// </summary>
        /// <param name="ipvx">remote endpoint</param>
        public void Connect(string ipvx)
        {
            lock (this)
            {
                // Create bridge
                _remote = new Bridge.CLR.RemoteClientBridge();
                
                // Always commit when remote append streams
                _remote.SetCommitOnAppend(true);

                // Install against endpoint
                bool state = _remote.Install(new Bridge.CLR.EndpointResolve()
                {
                    config = new Bridge.CLR.EndpointConfig()
                    {
                        applicationName = "Studio"
                    },
                    ipvxAddress = ipvx
                });

                // Invoke handlers
                if (state)
                {
                    Connected.OnNext(Unit.Default);
                }
                else
                {
                    Refused.OnNext(Unit.Default);
                }
            }
        }

        /// <summary>
        /// Connect asynchronously
        /// </summary>
        /// <param name="ipvx">remote endpoint</param>
        public void ConnectAsync(string ipvx)
        {
            _dispatcher.InvokeAsync(() => Connect(ipvx));
        }

        /// <summary>
        /// Submit discovery request asynchronously
        /// </summary>
        public void DiscoverAsync()
        {
            lock (this)
            {
                _remote?.DiscoverAsync();
            }
        }

        /// <summary>
        /// Submit client request asynchronously
        /// </summary>
        /// <param name="guid">endpoint application GUID</param>
        public void RequestClientAsync(ApplicationInfo applicationInfo)
        {
            lock (this)
            {
                // Set info
                Application = applicationInfo;
                
                // Pass down CLR
                _remote?.RequestClientAsync(applicationInfo.Guid);
            }
        }

        /// <summary>
        /// Get the shared bus within this connection
        /// </summary>
        /// <returns></returns>
        public OrderedMessageView<ReadWriteMessageStream> GetSharedBus()
        {
            // Allocate ordered if not ready
            if (_sharedBus == null)
            {
                _sharedBus = new OrderedMessageView<ReadWriteMessageStream>(new ReadWriteMessageStream());
            }

            // OK
            return _sharedBus;
        }

        /// <summary>
        /// Commit all pending bus messages
        /// </summary>
        public void Commit()
        {
            // Early out
            if (_sharedBus == null)
            {
                return;
            }
            
            // Submit current bus
            Bridge?.GetOutput().AddStream(_sharedBus.Storage);
            Bridge?.Commit();

            // Release
            _sharedBus = null;
        }

        /// <summary>
        /// Internal dispatcher
        /// </summary>
        private Dispatcher _dispatcher;

        /// <summary>
        /// Shared bus timer
        /// </summary>
        private DispatcherTimer _timer;

        /// <summary>
        /// Shared bus for convenient messaging
        /// </summary>
        private OrderedMessageView<ReadWriteMessageStream>? _sharedBus;
        
        /// <summary>
        /// Internal underlying bridge
        /// </summary>
        private Bridge.CLR.RemoteClientBridge? _remote;

        /// <summary>
        /// Internal local time
        /// </summary>
        private DateTime localTime;
    }
}