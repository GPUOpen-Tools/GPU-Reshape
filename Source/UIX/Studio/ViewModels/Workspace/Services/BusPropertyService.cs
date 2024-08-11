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
using System.Collections.Generic;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Workspace.Properties;
using Runtime.ViewModels.Workspace.Properties.Bus;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Services
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
        /// All version controllers
        /// </summary>
        public ISourceList<IBusVersionController> VersionControllers { get; } = new SourceList<IBusVersionController>();

        /// <summary>
        /// Constructor
        /// </summary>
        public BusPropertyService()
        {
            // Bind connection
            this.WhenAnyValue(x => x.ConnectionViewModel).Subscribe(_ => CreateVersionControllers());
        }

        /// <summary>
        /// Create all controllers
        /// </summary>
        private void CreateVersionControllers()
        {
            VersionControllers.Clear();
            
            // Default controllers
            VersionControllers.Add(new InstrumentationBusVersionController()
            {
                ConnectionViewModel = ConnectionViewModel
            });
        }

        /// <summary>
        /// Enqueue a given unique object
        /// </summary>
        /// <param name="busObject"></param>
        public void Enqueue(IBusObject busObject)
        {
            // In discard mode?
            if (Mode == BusMode.Discard)
            {
                return;
            }
            
            // No duplicates
            if (_lookup.Contains(busObject))
            {
                return;
            }

            // Immediate?
            if (Mode == BusMode.Immediate && ConnectionViewModel != null)
            {
                var stream = ConnectionViewModel.GetSharedBus();
                CommitVersionControllers(stream);
                busObject.Commit(stream);
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
            
            // Redirect to shared bus
            CommitRedirect(ConnectionViewModel.GetSharedBus(), true);
        }

        /// <summary>
        /// Destruct all items
        /// </summary>
        public void Destruct()
        {
            foreach (IBusVersionController controller in VersionControllers.Items)
            {
                controller.Destruct();
            }
        }

        /// <summary>
        /// Commit all outstanding objects to a given stream
        /// </summary>
        /// <param name="stream"></param>
        public void CommitRedirect(OrderedMessageView<ReadWriteMessageStream> stream, bool flush)
        {
            // Update all versions
            CommitVersionControllers(stream);
            
            // Commit all objects
            foreach (IBusObject objectsItem in Objects.Items)
            {
                objectsItem.Commit(stream);
            }

            // Flush
            if (flush)
            {
                Objects.Clear();
                _lookup.Clear();
            }
        }

        /// <summary>
        /// Commit all versions
        /// </summary>
        private void CommitVersionControllers(OrderedMessageView<ReadWriteMessageStream> stream)
        {
            foreach (IBusVersionController controller in VersionControllers.Items)
            {
                controller.Commit(stream);
            }
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