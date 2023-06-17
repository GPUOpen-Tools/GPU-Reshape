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
using ReactiveUI;
using Runtime.Models.Query;
using Studio.Models.Workspace;
using Studio.Models.Workspace.Objects;

namespace Studio.ViewModels.Query
{
    public class PipelineFilterQueryViewModel : ReactiveObject
    {
        /// <summary>
        /// Filter type
        /// </summary>
        public PipelineType? Type
        {
            get => _type;
            set => this.RaiseAndSetIfChanged(ref _type, value);
        }

        /// <summary>
        /// Filter name
        /// </summary>
        public string? Name
        {
            get => _name;
            set => this.RaiseAndSetIfChanged(ref _name, value);
        }

        /// <summary>
        /// Filter decorator
        /// </summary>
        public string Decorator
        {
            get => _decorator;
            set => this.RaiseAndSetIfChanged(ref _decorator, value);
        }
        
        /// <summary>
        /// Create a query from attributes
        /// </summary>
        /// <param name="attributes">given attributes</param>
        /// <param name="viewModel"></param>
        /// <returns>result code</returns>
        public static QueryResult FromAttributes(QueryAttribute[] attributes, out PipelineFilterQueryViewModel? viewModel)
        {
            QueryResult result = QueryParser.FromAttributes(new QueryType()
            {
                Variables = new()
                {
                    { "type", typeof(PipelineType) },
                    { "name", typeof(string) },
                }
            }, attributes, out QueryParser? query);

            // Success?
            if (result != QueryResult.OK)
            {
                viewModel = null;
                return result;
            }

            // Create VM
            viewModel = new PipelineFilterQueryViewModel()
            {
                Type = query.Get<PipelineType>("type"),
                Name = query.GetString("name")
            };

            // OK
            return QueryResult.OK;
        }
        
        /// <summary>
        /// Internal type
        /// </summary>
        private PipelineType? _type;

        /// <summary>
        /// Internal name
        /// </summary>
        private string? _name;

        /// <summary>
        /// Internal decorator
        /// </summary>
        private string _decorator = string.Empty;
    }
}