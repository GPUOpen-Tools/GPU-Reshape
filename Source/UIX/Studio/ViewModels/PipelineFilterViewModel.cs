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