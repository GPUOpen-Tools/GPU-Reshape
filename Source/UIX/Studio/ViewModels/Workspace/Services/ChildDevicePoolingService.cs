using System;
using System.Collections.Generic;
using System.Threading;
using Avalonia;
using Avalonia.Threading;
using Bridge.CLR;
using DynamicData;
using Message.CLR;
using Microsoft.Msagl.Core.DataStructures;
using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace.Services
{
    public class ChildDevicePoolingService : ReactiveObject, IPropertyService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Assigned token to listen for
        /// </summary>
        public string ReservedToken { get; set; }
        
        /// <summary>
        /// Property view model to extend with workspaces
        /// </summary>
        public IPropertyViewModel TargetViewModel { get; set; }
        
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

        public ChildDevicePoolingService(string reservedToken)
        {
            ReservedToken = reservedToken;
            
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

        /// <summary>
        /// Invoked on pooling ticks
        /// </summary>
        private void OnTick(object? sender, EventArgs e)
        {
            Refresh();
        }

        /// <summary>
        /// Re-discover all applications
        /// </summary>
        public void Refresh()
        {
            if (_connectionViewModel is IPoolingConnection poolingConnection)
            {
                poolingConnection.DiscoverAsync();
            }
        }

        /// <summary>
        /// Invoked on connection changes
        /// </summary>
        private void OnConnectionChanged()
        {
            _connectionViewModel?.Bridge?.Register(this);
        }
        
        /// <summary>
        /// Invoked on message streams
        /// </summary>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            var schema = new OrderedMessageView(streams);

            // Visit typed
            foreach (OrderedMessage message in schema)
            {
                switch (message.ID)
                {
                    case HostDiscoveryMessage.ID:
                    {
                        Handle(message.Get<HostDiscoveryMessage>());
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Find the next uncaptured application
        /// </summary>
        private ApplicationInfoViewModel? FindNextApplication(HostDiscoveryMessage discovery)
        {
            // Visit typed
            foreach (OrderedMessage message in new OrderedMessageView(discovery.infos.Stream))
            {
                switch (message.ID)
                {
                    case HostServerInfoMessage.ID:
                    {
                        var info = message.Get<HostServerInfoMessage>();
                        
                        // Matched against reserved?
                        if (!info.reservedGuid.String.Equals(ReservedToken, StringComparison.InvariantCultureIgnoreCase))
                        {
                            break;
                        }

                        // Ignore already caught devices (or previously closed)
                        if (_deviceGuids.Contains(info.guid.String))
                        {
                            break;
                        }

                        // Mark as acquired
                        _deviceGuids.Insert(info.guid.String);
                        
                        return new ApplicationInfoViewModel
                        {
                            Name = info.application.String,
                            Process = info.process.String,
                            API = info.api.String,
                            Pid = info.processId,
                            DeviceUid = info.deviceUid,
                            DeviceObjects = info.deviceObjects,
                            Guid = new Guid(info.guid.String),
                            DecorationMode = ApplicationDecorationMode.DeviceOnly
                        };
                    }
                }
            }

            // None found
            return null;
        }

        /// <summary>
        /// Handle a discovery message
        /// </summary>
        private void Handle(HostDiscoveryMessage discovery)
        {
            if (FindNextApplication(discovery) is not {} applicationInfo)
            {
                return;
            }

            // Create entry and kick off connection
            ChildDevicePoolingObject entry = new(_connectionViewModel, TargetViewModel, applicationInfo);

            // Keep track of it
            lock (_devices)
            {
                _devices.Add(applicationInfo.Guid.ToString(), entry);
            }
        }

        /// <summary>
        /// Internal connection state
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Currently allocated guids
        /// </summary>
        private Set<string> _deviceGuids = new();

        /// <summary>
        /// All tracked devices
        /// </summary>
        private Dictionary<string, ChildDevicePoolingObject> _devices = new();

        /// <summary>
        /// Pooling timer
        /// </summary>
        private DispatcherTimer _timer;
    }
}