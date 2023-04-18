using System;
using System.Collections.Generic;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Workspace.Properties;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace
{
    public class BusPropertyService : ReactiveObject, IBusPropertyService
    {
        /// <summary>
        /// Current bus mode
        /// </summary>
        public BusMode Mode
        {
            get => _mode; 
            set => this.RaiseAndSetIfChanged(ref _mode, value);
        }
        
        /// <summary>
        /// View model associated with this service
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set => this.RaiseAndSetIfChanged(ref _connectionViewModel, value);
        }

        /// <summary>
        /// All enqueued objects
        /// </summary>
        public ISourceList<IBusObject> Objects { get; } = new SourceList<IBusObject>();

        /// <summary>
        /// Enqueue a given unique object
        /// </summary>
        /// <param name="busObject"></param>
        public void Enqueue(IBusObject busObject)
        {
            // No duplicates
            if (_lookup.Contains(busObject))
            {
                return;
            }

            // Immediate?
            if (Mode == BusMode.Immediate && ConnectionViewModel != null)
            {
                busObject.Commit(ConnectionViewModel.GetSharedBus());
                return;
            }
            
            // Deferred, add object
            _lookup.Add(busObject);
            Objects.Add(busObject);
        }

        /// <summary>
        /// Commit all outstanding objects 
        /// </summary>
        public void Commit()
        {
            if (ConnectionViewModel == null)
            {
                return;
            }
            
            // Get bus
            OrderedMessageView<ReadWriteMessageStream> bus = ConnectionViewModel.GetSharedBus();
            
            // Commit all objects
            foreach (IBusObject objectsItem in Objects.Items)
            {
                objectsItem.Commit(bus);
            }
            
            // Flush
            Objects.Clear();
            _lookup.Clear();
        }
        
        /// <summary>
        /// Internal connection
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Lookup of enqueued bus objects
        /// </summary>
        private HashSet<IBusObject> _lookup = new HashSet<IBusObject>();

        /// <summary>
        /// Internal bus mode
        /// </summary>
        private BusMode _mode = BusMode.Immediate;
    }
}