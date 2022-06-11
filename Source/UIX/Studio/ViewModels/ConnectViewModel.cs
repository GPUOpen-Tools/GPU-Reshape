using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Windows.Input;
using Bridge.CLR;
using Dock.Model.Controls;
using Dock.Model.Core;
using DynamicData;
using Message.CLR;
using ReactiveUI;

namespace Studio.ViewModels
{
    public class ConnectViewModel : ReactiveObject, IBridgeListener
    {
        public Models.WorkspaceConnection Connection = new();

        public ObservableCollection<string> ResolvedApplications => _resolvedApplications;

        public ConnectViewModel()
        {
            // Subscribe
            Connection.Connected += OnConnected;
            
            // Start connection at localhost
            Connection.ConnectAsync("127.0.0.1");
        }

        private void OnConnected(Models.WorkspaceConnection _)
        {
            IBridge bridge = Connection.GetBridge();

            // Register handler
            bridge?.Register(this);
            
            // Request discovery
            Connection.DiscoverAsync();
        }
        
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            var schema = new OrderedMessageView(streams);

            foreach (OrderedMessage message in schema)
            {
                switch (message.ID)
                {
                    case HostDiscoveryMessage.ID:
                    {
                        HandleDiscovery(message.Get<HostDiscoveryMessage>());
                        break;
                    }
                }
            }
        }

        private void HandleDiscovery(HostDiscoveryMessage discovery)
        {
            var schema = new OrderedMessageView(discovery.infos.Stream);

            foreach (OrderedMessage message in schema)
            {
                switch (message.ID)
                {
                    case HostServerInfoMessage.ID:
                    {
                        var info = message.Get<HostServerInfoMessage>();
                        
                        _resolvedApplications.Add(info.application.String);
                        break;
                    }
                }
            }
        }
        
        // All remote resolved applications
        public ObservableCollection<string> _resolvedApplications = new();
    }
}
