using System;
using System.Collections.Generic;
using System.Threading;
using Avalonia;
using Avalonia.Threading;
using Bridge.CLR;
using DynamicData;
using Message.CLR;
using Studio.Services;
using Studio.Utils;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace.Services
{
    public class ChildDevicePoolingObject : IBridgeListener
    {
        public ChildDevicePoolingObject(IConnectionViewModel? rootConnectionViewModel, IPropertyViewModel targetViewModel, ApplicationInfoViewModel applicationInfo)
        {
            _applicationInfo = applicationInfo;
            _targetViewModel = targetViewModel;
            _rootConnectionViewModel = rootConnectionViewModel;
            
            // Create thread
            _thread = new Thread(CreateEndpointThread);

            // Create connection on main thread to ensure the right dispatcher is assigned
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                _connection = new();
                _thread.Start();
            });
        }

        /// <summary>
        /// Thread entry point for endpoint creation
        /// </summary>
        private void CreateEndpointThread(object? opaqueEntry)
        {
            // Once connected, immediately request the target application
            _connection.Connected.Subscribe(_ =>
            {
                _connection.Bridge?.Register(this);
                _connection.RequestClientAsync(_applicationInfo);
            });
            
            // Create connection
            _connection.Connect("127.0.0.1", null);
        }
        
        /// <summary>
        /// Invoked on bridge handlers
        /// </summary>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            foreach (OrderedMessage message in new OrderedMessageView(streams))
            {
                switch (message.ID)
                {
                    case HostConnectedMessage.ID:
                    {
                        Dispatcher.UIThread.InvokeAsync(OnHostConnected);
                        break;
                    }
                }
            }
        }

        /// <summary>
        /// Invoked on host connections
        /// </summary>
        private void OnHostConnected()
        {
            // Get provider
            var provider = ServiceRegistry.Get<IWorkspaceService>();

            // Create workspace
            var workspace = new WorkspaceViewModel()
            {
                Connection = _connection
            };
            
            // Configure and register workspace
            provider?.Install(workspace);

            // Create the process tree
            IPropertyViewModel targetPropertyViewModel = CreateProcessTree();
            
            // Add to target
            targetPropertyViewModel.Properties.Add(workspace.PropertyCollection);
            workspace.PropertyCollection.Parent = targetPropertyViewModel;
        }

        /// <summary>
        /// Create the final process tree
        /// </summary>
        /// <returns>leaf node in which to create the device</returns>
        private IPropertyViewModel CreateProcessTree()
        {
            List<Tuple<long, string>> tree = GetAscendingProcessTree((long)(_rootConnectionViewModel?.Application?.Pid ?? 0), (long)_applicationInfo.Pid);
            
            // Create ascending tree
            IPropertyViewModel propertyNodeViewModel = _targetViewModel;
            foreach (Tuple<long, string> process in tree)
            {
                // Try to find a base node with the same id
                var nextNodeViewModel = propertyNodeViewModel.GetPropertyWhere<ProcessNodeViewModel>(P => P.PId == process.Item1);
                if (nextNodeViewModel == null)
                {
                    // None found, instantiate, may be detached
                    propertyNodeViewModel.Properties.Add(nextNodeViewModel = new ProcessNodeViewModel(process.Item2)
                    {
                        PId = process.Item1,
                        Parent = propertyNodeViewModel
                    });
                }

                // Next!
                propertyNodeViewModel = nextNodeViewModel;
            }

            // OK
            return propertyNodeViewModel;
        }

        /// <summary>
        /// Get the ascending process tree, base -> child -> leaf
        /// </summary>
        /// <param name="root">the base identifier to search for</param>
        /// <param name="last">the leaf node to traverse from</param>
        /// <returns>ascending tree</returns>
        private List<Tuple<long, string>> GetAscendingProcessTree(long root, long last)
        {
            List<Tuple<long, string>> chain = new();

            // Get all parents
            while (last != root)
            {
                string name;
                try
                {
                    name = ProcessWin32.GetProcess((int)last)?.ProcessName ?? "Detached";
                }
                catch (InvalidOperationException)
                {
                    name = "Detached";
                }
                
                chain.Add(Tuple.Create(last, $"{name} - {last}"));

                // If detached, stop traversing
                if (last == 0)
                {
                    break;
                }
                
                last = ProcessWin32.GetParentProcess((int)last)?.Id ?? 0;
            }
            
            // Reverse in place, descending -> ascending
            chain.Reverse();
            return chain;
        }

        /// <summary>
        /// Owning thread
        /// </summary>
        private Thread _thread;

        /// <summary>
        /// Property to be added to
        /// </summary>
        private IPropertyViewModel _targetViewModel;

        /// <summary>
        /// Internal connection, fed to workspace
        /// </summary>
        private ConnectionViewModel _connection;

        /// <summary>
        /// The root connection
        /// </summary>
        private IConnectionViewModel? _rootConnectionViewModel;

        /// <summary>
        /// Assigned application info
        /// </summary>
        private ApplicationInfoViewModel _applicationInfo;
    }
}