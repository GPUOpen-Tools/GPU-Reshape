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
using ReactiveUI;
using Runtime.Models.Query;

namespace Studio.ViewModels.Workspace.Message
{
    public class HierarchicalMessageQueryViewModel : ReactiveObject
    {
        /// <summary>
        /// General query that applies to all textual parts
        /// </summary>
        public string? GeneralQuery
        {
            get => _generalQuery;
            set => this.RaiseAndSetIfChanged(ref _generalQuery, value);
        }
        
        /// <summary>
        /// Shader name query
        /// </summary>
        public string? Shader
        {
            get => _shader;
            set => this.RaiseAndSetIfChanged(ref _shader, value);
        }

        /// <summary>
        /// Validation message query
        /// </summary>
        public string? Message
        {
            get => _message;
            set => this.RaiseAndSetIfChanged(ref _message, value);
        }

        /// <summary>
        /// Code extract query
        /// </summary>
        public string? Code
        {
            get => _code;
            set => this.RaiseAndSetIfChanged(ref _code, value);
        }

        /// <summary>
        /// Create a query from attributes
        /// </summary>
        public static HierarchicalMessageQueryViewModel? FromAttributes(QueryAttribute[] attributes)
        {
            QueryResult result = QueryParser.FromAttributes(new QueryType()
            {
                Variables = new()
                {
                    { QueryParser.UntypedKey, typeof(string) },
                    { "shader", typeof(string) },
                    { "message", typeof(string) },
                    { "code", typeof(string) }
                }
            }, attributes, out QueryParser? query);

            // Handle result
            switch (result)
            {
                case QueryResult.OK:
                    break;
                case QueryResult.Invalid:
                    return null;
                case QueryResult.DuplicateKey:
                    return null;
                default:
                    throw new ArgumentOutOfRangeException();
            }

            // Failed?
            if (query == null)
            {
                return null;
            }

            // Create VM
            return new HierarchicalMessageQueryViewModel()
            {
                GeneralQuery = query.GetString(QueryParser.UntypedKey),
                Shader = query.GetString("shader"),
                Message = query.GetString("message"),
                Code = query.GetString("code")
            };
        }

        /// <summary>
        /// Internal general
        /// </summary>
        private string? _generalQuery;
        
        /// <summary>
        /// Internal shader query
        /// </summary>
        private string? _shader;

        /// <summary>
        /// Internal message query
        /// </summary>
        private string? _message;

        /// <summary>
        /// Internal code query
        /// </summary>
        private string? _code;
    }   
}