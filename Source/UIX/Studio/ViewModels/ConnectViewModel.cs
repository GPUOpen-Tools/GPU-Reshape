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

            for (OrderedMessageIterator it = schema.GetIterator(); it.Good(); it.Next())
            {
                switch (it.GetMessageID())
                {
                    case HostDiscoveryMessage.ID:
                    {
                        var message = it.Get<HostDiscoveryMessage>();
                        break;
                    }
                }
            }
        }
        
        // All remote resolved applications
        public ObservableCollection<string> _resolvedApplications = new();
    }
}
