// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Avalonia.Threading;
using Bridge.CLR;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using DynamicData.Binding;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Runtime.ViewModels.Objects;
using Runtime.ViewModels.Tools;
using Studio.Models.Logging;
using Studio.Services;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Tools
{
    public class PipelineTreeViewModel : ToolViewModel, IBridgeListener
    {
        /// <summary>
        /// Tooling icon
        /// </summary>
        public override StreamGeometry? Icon => ResourceLocator.GetIcon("ToolPipelineTree");
        
        /// <summary>
        /// All identifiers
        /// </summary>
        public ObservableCollection<PipelineIdentifierViewModel> PipelineIdentifiers { get; } = new();

        /// <summary>
        /// Is the help message visible?
        /// </summary>
        public bool IsHelpVisible
        {
            get => PipelineIdentifiers.Count == 0;
        }

        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IWorkspaceViewModel? WorkspaceViewModel
        {
            get => _workspaceViewModel;
            set
            {
                DestructConnection();
                
                this.RaiseAndSetIfChanged(ref _workspaceViewModel, value);

                OnConnectionChanged();
            }
        }

        /// <summary>
        /// Refresh all items
        /// </summary>
        public ICommand Refresh { get; }
        
        /// <summary>
        /// Opens a given pipeline document from view model
        /// </summary>
        public ICommand OpenPipelineDocument;

        public PipelineTreeViewModel()
        {
            Refresh = ReactiveCommand.Create(OnRefresh);
            OpenPipelineDocument = ReactiveCommand.Create<PipelineIdentifierViewModel>(OnOpenPipelineDocument);
            
            // Bind visibility
            PipelineIdentifiers.ToObservableChangeSet(x => x)
                .OnItemAdded(_ => this.RaisePropertyChanged(nameof(IsHelpVisible)))
                .Subscribe();
            
            // Bind selected workspace
            App.Locator.GetService<IWorkspaceService>()?
                .WhenAnyValue(x => x.SelectedWorkspace)
                .Subscribe(x => WorkspaceViewModel = x);
        }

        /// <summary>
        /// Invoked on document handlers
        /// </summary>
        /// <param name="pipelineIdentifierViewModel"></param>
        private void OnOpenPipelineDocument(PipelineIdentifierViewModel pipelineIdentifierViewModel)
        {
            // TODO: ...
        }

        /// <summary>
        /// Populate a given range
        /// </summary>
        /// <param name="start">starting index</param>
        /// <param name="end">end index</param>
        public void PopulateRange(int start, int end)
        {
            if (_workspaceViewModel is not { Connection: { } })
                return;
            
            // Submit range request
            var range = _workspaceViewModel.Connection.GetSharedBus().Add<GetPipelineUIDRangeMessage>();
            range.start = (uint)start;
            range.limit = (uint)(end - start);
        }

        /// <summary>
        /// Populate a single identifier
        /// </summary>
        /// <param name="pipelineIdentifierViewModel"></param>
        public void PopulateSingle(PipelineIdentifierViewModel pipelineIdentifierViewModel)
        {
            if (_workspaceViewModel is not { Connection: { } })
                return;
            
            // Submit single request
            var range = _workspaceViewModel.Connection.GetSharedBus().Add<GetPipelineUIDRangeMessage>();
            range.start = (uint)PipelineIdentifiers.IndexOf(pipelineIdentifierViewModel);
            range.limit = 1;
        }

        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            // Clear states
            PipelineIdentifiers.Clear();
            _lookup.Clear();
            
            if (_workspaceViewModel is not { Connection: { } })
                return;
            
            // Subscribe
            _workspaceViewModel.Connection.Bridge?.Register(this);

            // Start pooling timer
            _poolTimer = new DispatcherTimer(TimeSpan.FromSeconds(1), DispatcherPriority.Background, OnPoolEvent);
            _poolTimer.Start();
        }
        
        /// <summary>
        /// Destruct an existing connection
        /// </summary>
        private void DestructConnection()
        {
            if (_workspaceViewModel is not { Connection: { } })
                return;
            
            // Unsubscribe
            _workspaceViewModel.Connection.Bridge?.Deregister(this);
        }
        
        /// <summary>
        /// Invoked on timed pooling
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void OnPoolEvent(object? sender, EventArgs e)
        {
            // Submit request for all objects
            _workspaceViewModel?.Connection?.GetSharedBus().Add<GetObjectStatesMessage>();
        }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsOrdered())
            {
                return;
            }

            // Enumerate all messages
            foreach (OrderedMessage message in new OrderedMessageView(streams))
            {
                switch (message.ID)
                {
                    case PipelineNameMessage.ID:
                        Handle(message.Get<PipelineNameMessage>());
                        break;
                    case ObjectStatesMessage.ID:
                        Handle(message.Get<ObjectStatesMessage>());
                        break;
                    case PipelineUIDRangeMessage.ID:
                        Handle(message.Get<PipelineUIDRangeMessage>());
                        break;
                }
            }
        }

        /// <summary>
        /// Message handler
        /// </summary>
        /// <param name="message"></param>
        private void Handle(PipelineNameMessage message)
        {
            // Flatten dynamics
            UInt64 uid = message.pipelineUID;
            string name = message.name.String;

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (!_lookup.ContainsKey(uid))
                    return;

                PipelineIdentifierViewModel pipelineIdentifierViewModel = _lookup[uid];
                pipelineIdentifierViewModel.Descriptor = $"Pipeline {uid} - {name}";
            });
        }

        /// <summary>
        /// Message handler
        /// </summary>
        /// <param name="message"></param>
        private void Handle(ObjectStatesMessage message)
        {
            // Flatten message
            ObjectStatesMessage.FlatInfo flat = message.Flat;

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                // Prune dead models
                for (uint i = flat.pipelineCount; i < PipelineIdentifiers.Count; i++)
                {
                    PipelineIdentifierViewModel pipelineIdentifierViewModel = PipelineIdentifiers[(int)i];

                    // Remove GUID association if needed
                    if (pipelineIdentifierViewModel.GUID != 0)
                    {
                        _lookup.Remove(pipelineIdentifierViewModel.GUID);
                    }
                    
                    PipelineIdentifiers.RemoveAt((int)i);
                }

                // Add new models
                for (int i = PipelineIdentifiers.Count; i < flat.pipelineCount; i++)
                {
                    PipelineIdentifiers.Add(new()
                    {
                        Workspace = WorkspaceViewModel,
                        GUID = 0,
                        Descriptor = "Loading..."
                    });
                }
            });
        }

        /// <summary>
        /// Message handler
        /// </summary>
        /// <param name="message"></param>
        private void Handle(PipelineUIDRangeMessage message)
        {
            uint start = message.start;
            
            var guids = new UInt64[message.pipelineUID.Count];

            // Copy guids
            for (int i = 0; i < message.pipelineUID.Count; i++)
            {
                guids[i] = message.pipelineUID[i];
            }

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                var bus = _workspaceViewModel?.Connection?.GetSharedBus();

                for (int i = 0; i < guids.Length; i++)
                {
                    if (_lookup.ContainsKey(guids[i]))
                    {
                        // OK
                    }
                    else if (start + i < PipelineIdentifiers.Count)
                    {
                        PipelineIdentifierViewModel pipelineIdentifierViewModel = PipelineIdentifiers[(int)start + i];

                        // Update identifier
                        pipelineIdentifierViewModel.GUID = guids[i];
                        pipelineIdentifierViewModel.Descriptor = $"Pipeline {guids[i]}";

                        // Submit request for debug name
                        if (bus != null)
                        {
                            var request = bus.Add<GetPipelineNameMessage>();
                            request.pipelineUID = guids[i];
                        }
                        
                        // Add association
                        _lookup.Add(guids[i], pipelineIdentifierViewModel);
                    }
                }
            });
        }

        /// <summary>
        /// Invoked on refresh requests
        /// </summary>
        /// <param name="message"></param>
        private void OnRefresh()
        {
            // Clear states
            PipelineIdentifiers.Clear();
            _lookup.Clear();
            
            // Immediate repool
            _workspaceViewModel?.Connection?.GetSharedBus().Add<GetObjectStatesMessage>();
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private IWorkspaceViewModel? _workspaceViewModel;

        /// <summary>
        /// Internal GUID association
        /// </summary>
        private Dictionary<UInt64, PipelineIdentifierViewModel> _lookup = new();

        /// <summary>
        /// Internal pooling timer
        /// </summary>
        private DispatcherTimer? _poolTimer;
    }
}