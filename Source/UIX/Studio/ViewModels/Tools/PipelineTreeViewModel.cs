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
using System.Collections.ObjectModel;
using System.Reactive.Linq;
using System.Windows.Input;
using Avalonia.Media;
using Avalonia.Threading;
using Bridge.CLR;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Query;
using Runtime.ViewModels.Objects;
using Runtime.ViewModels.Tools;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
using Studio.Services;
using Studio.Services.Suspension;
using Studio.ViewModels.Query;
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
        /// Tooling tip
        /// </summary>
        public override string? ToolTip => Resources.Resources.Tool_Pipelines;
        
        /// <summary>
        /// All identifiers
        /// </summary>
        public ObservableCollection<PipelineIdentifierViewModel> PipelineIdentifiers { get; } = new();
        
        /// <summary>
        /// All filtered identifiers
        /// </summary>
        public ObservableCollection<PipelineIdentifierViewModel> FilteredIdentifiers { get; } = new();
        
        /// <summary>
        /// All visible and pooled identifiers
        /// </summary>
        public ObservableCollection<PipelineIdentifierViewModel> PooledIdentifiers { get; } = new();

        /// <summary>
        /// Is the help message visible?
        /// </summary>
        public bool IsHelpVisible
        {
            get => FilteredIdentifiers.Count == 0;
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
        /// Current filter string
        /// </summary>
        [DataMember]
        public string FilterString
        {
            get => _filterString;
            set => this.RaiseAndSetIfChanged(ref _filterString, value);
        }

        /// <summary>
        /// Parsed filter query
        /// </summary>
        public PipelineQueryViewModel? FilterQuery
        {
            get => _filterQuery;
            set => this.RaiseAndSetIfChanged(ref _filterQuery, value);
        }

        /// <summary>
        /// Current filter status
        /// </summary>
        public QueryResult FilterStatus
        {
            get => _filterStatus;
            set => this.RaiseAndSetIfChanged(ref _filterStatus, value);
        }

        /// <summary>
        /// All string decorators
        /// </summary>
        public SourceList<QueryAttributeDecorator> QueryDecorators
        {
            get => _queryDecorators;
            set => this.RaiseAndSetIfChanged(ref _queryDecorators, value);
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
            
            // Initialize filter status
            FilterStatus = QueryResult.OK;
            
            // Bind selected workspace
            ServiceRegistry.Get<IWorkspaceService>()?
                .WhenAnyValue(x => x.SelectedWorkspace)
                .Subscribe(x => WorkspaceViewModel = x);

            // Notify on query string change
            this.WhenAnyValue(x => x.FilterString)
                .Throttle(TimeSpan.FromMilliseconds(250))
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(CreateFilterQuery);
            
            // Create timer on main thread
            _poolingTimer = new DispatcherTimer(DispatcherPriority.Background)
            {
                Interval = TimeSpan.FromMilliseconds(500),
                IsEnabled = true
            };

            // Subscribe tick
            _poolingTimer.Tick += OnPoolingTick;

            // Must call start manually (a little vague)
            _poolingTimer.Start();
            
            // Suspension
            this.BindTypedSuspension();
        }

        /// <summary>
        /// Invoked on ticks
        /// </summary>
        private void OnPoolingTick(object? sender, EventArgs e)
        {
            if (_workspaceViewModel is not { Connection: { } } || PooledIdentifiers.Count == 0)
                return;
            
            // Create request
            var message = _workspaceViewModel.Connection.GetSharedBus().Add<GetPipelineStatusMessage>(new GetPipelineStatusMessage.AllocationInfo
            {
                pipelineUIDsCount = (ulong)PooledIdentifiers.Count
            });

            // Fill all UIDs
            for (var i = 0; i < PooledIdentifiers.Count; i++)
            {
                message.pipelineUIDs.SetValue(i, PooledIdentifiers[i].GUID);
            }
        }

        /// <summary>
        /// Pool an immediate identifier
        /// </summary>
        public void PoolImmediate(PipelineIdentifierViewModel identifierViewModel)
        {
            if (_workspaceViewModel is not { Connection: { } })
                return;

            // Submit it immediately
            var message = _workspaceViewModel.Connection.GetSharedBus().Add<GetPipelineStatusMessage>(new GetPipelineStatusMessage.AllocationInfo { pipelineUIDsCount = 1 });
            message.pipelineUIDs.SetValue(0, identifierViewModel.GUID);
        }

        /// <summary>
        /// Update help visibility
        /// </summary>
        private void UpdateHelp()
        {
            this.RaisePropertyChanged(nameof(IsHelpVisible));
        }

        /// <summary>
        /// Disable all filtering
        /// </summary>
        private void DisableFilter()
        {
            FilterQuery = null;
            
            // Stop auto-population
            _alwaysPopulate = false;
            
            // Recreate identifiers
            RefilterPipelines();
        }

        /// <summary>
        /// Create a new filter query
        /// </summary>
        /// <param name="query">given query</param>
        private void CreateFilterQuery(string query)
        {
            if (string.IsNullOrWhiteSpace(query))
            {
                DisableFilter();
                return;
            }
            
            // Try to parse collection
            QueryAttribute[]? attributes = QueryParser.GetAttributes(query, true);
            if (attributes == null)
            {
                FilterStatus = QueryResult.Invalid;
                
                // Something went wrong, disable
                DisableFilter();
                return;
            }

            // We've started a query, request complete population of the entire missing range
            // If sparse, this will request redundant identifiers
            PopulatePendingRange();

            // Create new segment list
            QueryDecorators.Clear();

            // Visualize segments
            foreach (QueryAttribute attribute in attributes)
            {
                QueryDecorators.Add(new QueryAttributeDecorator()
                {
                    Attribute = attribute,
                    Color = attribute.Key switch
                    {
                        "type" => ResourceLocator.GetResource<Color>("PipelineFilterKeyType"),
                        _ => ResourceLocator.GetResource<Color>("PipelineFilterKeyName")
                    }
                });
            }

            // Attempt to parse
            FilterStatus = PipelineQueryViewModel.FromAttributes(attributes, out var queryObject);

            // Set decorator
            if (queryObject != null)
            {
                queryObject.Decorator = query;
            }

            // Set query
            FilterQuery = queryObject;

            // Auto-populate any future identifier
            _alwaysPopulate = true;
            
            // Refilter if needed
            RefilterPipelines();
        }

        /// <summary>
        /// Refilter all pipelines
        /// </summary>
        private void RefilterPipelines()
        {
            // Clear previous range
            FilteredIdentifiers.Clear();
            _filterSet.Clear();

            // If no query, just add them all
            if (FilterQuery == null)
            {
                FilteredIdentifiers.AddRange(PipelineIdentifiers);
            }
            else
            {
                PipelineIdentifiers.ForEach(x => FilterPipeline(x));
            }

            UpdateHelp();
        }

        /// <summary>
        /// Filter a specific pipeline
        /// Adds it if passed
        /// </summary>
        private bool FilterPipeline(PipelineIdentifierViewModel identifierViewModel)
        {
            // If already handled, ignore
            if (_filterSet.Contains(identifierViewModel))
            {
                return true;
            }
            
            // If there's no query, just mark it as visible
            if (FilterQuery == null)
            {
                // No need to add it to the set, cleared on query construction
                FilteredIdentifiers.Add(identifierViewModel);
                return true;
            }

            // Matching name?
            if (!string.IsNullOrEmpty(FilterQuery.Name) && !identifierViewModel.Descriptor.Contains(FilterQuery.Name, StringComparison.InvariantCultureIgnoreCase))
            {
                return false;
            }

            // Type?
            if (FilterQuery.Type.HasValue && identifierViewModel.Stage != FilterQuery.Type)
            {
                return false;
            }
            
            // Passed, mark as filtered
            FilteredIdentifiers.Add(identifierViewModel);
            _filterSet.Add(identifierViewModel);
            return true;
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
            FilteredIdentifiers.Clear();
            PipelineIdentifiers.Clear();
            _lookup.Clear();
            UpdateHelp();
            
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
        /// Populate a missing range
        /// </summary>
        private void PopulatePendingRange()
        {
            int start = int.MaxValue;
            int end = 0;
            
            // Find the range bounds
            for (int i = 0; i < PipelineIdentifiers.Count; i++)
            {
                if (PipelineIdentifiers[i].GUID == ulong.MaxValue)
                {
                    start = int.Min(start, i);
                    end = int.Max(end, i);
                }
            }

            // Invalid if no missing entries
            if (start != int.MaxValue)
            {
                PopulateRange(start, end);
            }
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
                    case PipelineStatusCollectionMessage.ID:
                        Handle(message.Get<PipelineStatusCollectionMessage>());
                        break;
                }
            }
        }

        /// <summary>
        /// Message handler
        /// </summary>
        private void Handle(PipelineStatusCollectionMessage message)
        {
            List<PipelineStatusMessage.FlatInfo> list = new();
            
            // Flatten pipelines
            foreach (PipelineStatusMessage status in new StaticMessageView<PipelineStatusMessage>(message.status.Stream))
            {
                list.Add(status.Flat);
            }
            
            // Update all active states on the UI thread
            Dispatcher.UIThread.InvokeAsync(() =>
            {
                foreach (PipelineStatusMessage.FlatInfo info in list)
                {
                    if (_lookup.TryGetValue(info.pipelineUID, out PipelineIdentifierViewModel? value))
                    {
                        value.Active = info.active == 1;
                        value.Instrumented = info.instrumented == 1;
                    }
                }
            });
        }

        /// <summary>
        /// Message handler
        /// </summary>
        /// <param name="message"></param>
        private void Handle(PipelineNameMessage message)
        {
            // Flatten dynamics
            PipelineNameMessage.FlatInfo flat = message.Flat;

            Dispatcher.UIThread.InvokeAsync(() =>
            {
                if (!_lookup.ContainsKey(flat.pipelineUID))
                    return;

                PipelineIdentifierViewModel pipelineIdentifierViewModel = _lookup[flat.pipelineUID];
                pipelineIdentifierViewModel.Descriptor = $"{flat.name} {flat.pipelineUID}";
                pipelineIdentifierViewModel.Stage = (PipelineType)flat.type;
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

                    // TODO: Expensive
                    FilteredIdentifiers.Remove(pipelineIdentifierViewModel);
                }

                // Immediately populate if needed
                if (_alwaysPopulate)
                {
                    PopulateRange(PipelineIdentifiers.Count, (int)flat.pipelineCount);
                }

                // Add new models
                for (int i = PipelineIdentifiers.Count; i < flat.pipelineCount; i++)
                {
                    PipelineIdentifierViewModel viewModel = new()
                    {
                        Workspace = WorkspaceViewModel,
                        GUID = ulong.MaxValue,
                        Descriptor = "Loading..."
                    };

                    // Bind filtering events
                    BindFiltering(viewModel);
                    
                    PipelineIdentifiers.Add(viewModel);
                }
            
                // Update label
                UpdateHelp();
            });
        }

        /// <summary>
        /// Bind all filtering events
        /// </summary>
        private void BindFiltering(PipelineIdentifierViewModel viewModel)
        {
            // Try to filter immediately
            if (FilterPipeline(viewModel))
            {
                return;
            }

            // Otherwise, bind for future changes
            viewModel
                .WhenAnyValue(x => x.Descriptor)
                .Subscribe(x => FilterPipeline(viewModel));
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
            
                UpdateHelp();
            });
        }

        /// <summary>
        /// Invoked on refresh requests
        /// </summary>
        /// <param name="message"></param>
        private void OnRefresh()
        {
            // Clear states
            FilteredIdentifiers.Clear();
            PipelineIdentifiers.Clear();
            _lookup.Clear();
            UpdateHelp();
            
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
        /// Set of filtered pipelines
        /// </summary>
        private HashSet<PipelineIdentifierViewModel> _filterSet = new();

        /// <summary>
        /// Internal pooling timer
        /// </summary>
        private DispatcherTimer? _poolTimer;

        /// <summary>
        /// Internal, default, connection string
        /// </summary>
        private string _filterString = "";

        /// <summary>
        /// Internal filter query
        /// </summary>
        private PipelineQueryViewModel? _filterQuery;

        /// <summary>
        /// Internal decorators
        /// </summary>
        private SourceList<QueryAttributeDecorator> _queryDecorators = new();

        /// <summary>
        /// Internal status
        /// </summary>
        private QueryResult _filterStatus;

        /// <summary>
        /// Always populate identifiers on discovery?
        /// </summary>
        private bool _alwaysPopulate = true;

        /// <summary>
        /// Timer for pooling
        /// </summary>
        private readonly DispatcherTimer _poolingTimer;
    }
}