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
using System.Drawing;
using System.Linq;
using System.Reactive.Disposables;
using System.Reactive.Linq;
using System.Text.RegularExpressions;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using Avalonia.Threading;
using Bridge.CLR;
using DynamicData;
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Query;
using Studio.Extensions;
using Studio.Models.Workspace;
using Studio.Services;
using Studio.Services.Suspension;
using Studio.ViewModels.Query;
using Studio.ViewModels.Workspace.Properties;
using Color = Avalonia.Media.Color;

namespace Studio.ViewModels
{
    public class PipelineFilterViewModel : ReactiveObject
    {
        /// <summary>
        /// Connect to selected application
        /// </summary>
        public ICommand Create { get; }

        /// <summary>
        /// VM activator
        /// </summary>
        public ViewModelActivator Activator { get; } = new();

        /// <summary>
        /// Given pipeline collection
        /// </summary>
        public IPipelineCollectionViewModel PipelineCollectionViewModel { get; set; }

        /// <summary>
        /// Current connection string
        /// </summary>
        [DataMember]
        public string FilterString
        {
            get => _filterString;
            set => this.RaiseAndSetIfChanged(ref _filterString, value);
        }

        /// <summary>
        /// Parsed connection query
        /// </summary>
        public PipelineFilterQueryViewModel? FilterQuery
        {
            get => _filterQuery;
            set => this.RaiseAndSetIfChanged(ref _filterQuery, value);
        }
        
        /// <summary>
        /// User interaction during connection acceptance
        /// </summary>
        public Interaction<PipelineFilterViewModel, bool> AcceptFilter { get; }

        /// <summary>
        /// Current query status
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
        /// Constructor
        /// </summary>
        public PipelineFilterViewModel()
        {
            // Initialize connection status
            FilterStatus = QueryResult.OK;

            // Create commands
            Create = ReactiveCommand.Create(OnConnect);

            // Create interactions
            AcceptFilter = new();

            // Notify on query string change
            this.WhenAnyValue(x => x.FilterString)
                .Throttle(TimeSpan.FromMilliseconds(250))
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(CreateConnectionQuery);
            
            // Suspension
            this.BindTypedSuspension();
        }

        /// <summary>
        /// Refresh the current query
        /// </summary>
        public void RefreshQuery()
        {
            CreateConnectionQuery(FilterString);
        }

        /// <summary>
        /// Create a new connection query
        /// </summary>
        /// <param name="query">given query</param>
        private void CreateConnectionQuery(string query)
        {
            // Try to parse collection
            QueryAttribute[]? attributes = QueryParser.GetAttributes(query);
            if (attributes == null)
            {
                FilterStatus = QueryResult.Invalid;
                return;
            }

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
            FilterStatus = PipelineFilterQueryViewModel.FromAttributes(attributes, out var queryObject);

            // Set decorator
            if (queryObject != null)
            {
                queryObject.Decorator = query;
            }

            // Set query
            FilterQuery = queryObject;
        }

        /// <summary>
        /// Connect implementation
        /// </summary>
        private async void OnConnect()
        {
            if (FilterStatus != QueryResult.OK)
            {
                return;
            }

            // Confirm with view
            if (!await AcceptFilter.Handle(this))
            {
                return;
            }
            
            // Create filter
            PipelineCollectionViewModel.Properties.Add(new Workspace.Properties.Instrumentation.PipelineFilterViewModel()
            {
                Parent = PipelineCollectionViewModel,
                ConnectionViewModel = PipelineCollectionViewModel.ConnectionViewModel,
                Filter = FilterQuery!
            });
        }

        /// <summary>
        /// Internal, default, connection string
        /// </summary>
        private string _filterString = "type:compute";

        /// <summary>
        /// Internal filter query
        /// </summary>
        private PipelineFilterQueryViewModel? _filterQuery;

        /// <summary>
        /// Internal decorators
        /// </summary>
        private SourceList<QueryAttributeDecorator> _queryDecorators = new();

        /// <summary>
        /// Internal status
        /// </summary>
        private QueryResult _filterStatus;
    }
}