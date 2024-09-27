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
using System.Reactive.Linq;
using Avalonia.Media;
using DynamicData;
using ReactiveUI;
using Runtime.Models.Query;

namespace Studio.ViewModels.Workspace.Message
{
    public class HierarchicalMessageQueryFilterViewModel : ReactiveObject
    {
        /// <summary>
        /// Current query string
        /// </summary>
        public string QueryString
        {
            get => _queryString;
            set => this.RaiseAndSetIfChanged(ref _queryString, value);
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
        /// Current query view model, updated on changes
        /// </summary>
        public HierarchicalMessageQueryViewModel? QueryViewModel
        {
            get => _queryViewModel;
            set => this.RaiseAndSetIfChanged(ref _queryViewModel, value);
        }

        public HierarchicalMessageQueryFilterViewModel()
        {
            // Notify on query string change
            this.WhenAnyValue(x => x.QueryString)
                .Throttle(TimeSpan.FromMilliseconds(500))
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(CreateQuery);
        }

        /// <summary>
        /// Refresh the current query
        /// </summary>
        public void RefreshQuery()
        {
            CreateQuery(QueryString);
        }

        /// <summary>
        /// Create a new query
        /// </summary>
        /// <param name="query">given query</param>
        private void CreateQuery(string query)
        {
            // Try to parse collection
            QueryAttribute[]? attributes = QueryParser.GetAttributes(query, true);
            if (attributes == null)
            {
                QueryViewModel = null;
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
                        "shader" => ResourceLocator.GetResource<Color>("ConnectionKeyPort"),
                        "message" => ResourceLocator.GetResource<Color>("ConnectionKeyApplicationFilter"),
                        "code" => ResourceLocator.GetResource<Color>("ConnectionKeyApplicationPID"),
                        _ => ResourceLocator.GetResource<Color>("ConnectionKeyIP")
                    }
                });
            }

            // Null query is a fast path
            if (attributes.Length == 0)
            {
                QueryViewModel = null;
                return;
            }

            // Attempt to parse
            QueryViewModel = HierarchicalMessageQueryViewModel.FromAttributes(attributes);
        }

        /// <summary>
        /// Internal query state
        /// </summary>
        private string _queryString = string.Empty;

        /// <summary>
        /// Internal decorators
        /// </summary>
        private SourceList<QueryAttributeDecorator> _queryDecorators = new();

        /// <summary>
        /// Internal query view model
        /// </summary>
        private HierarchicalMessageQueryViewModel? _queryViewModel;
    }   
}