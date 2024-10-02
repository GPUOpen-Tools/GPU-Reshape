﻿// 
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
using System.Linq;
using Avalonia.Controls;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Workspace.Message;

namespace Studio.Views.Controls
{
    public partial class MessageCollectionQueryView : UserControl
    {
        /// <summary>
        /// Constructor
        /// </summary>
        public MessageCollectionQueryView()
        {
            InitializeComponent();

            // Setup query
            QueryBox.ItemsSource = AutoSource;
            QueryBox.TextFilter = QueryTextFilter;
            QueryBox.TextSelector = QuerySelector;
            
            // Set query
            AttributeSegments.QueryBox = QueryBox;
            
            // On data context
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<HierarchicalMessageQueryFilterViewModel>()
                .Subscribe(viewModel =>
                {
                    // Set decorators
                    AttributeSegments.Decorators = viewModel.QueryDecorators;
                });
        }

        /// <summary>
        /// Invoked on filter additions
        /// </summary>
        /// <param name="search">search text</param>
        /// <param name="item">current suggestion</param>
        /// <returns></returns>
        private bool QueryTextFilter(string search, string item)
        {
            // Do not suggest items already decorated
            if (VM?.QueryDecorators.Items.Any(x => x.Attribute.Key == item.Substring(0, item.Length - 1)) ?? false)
            {
                return false;
            }

            // Compare last word
            string word = search.Split(' ').LastOrDefault() ?? search;
            return item.StartsWith(word.ToLower());
        }

        /// <summary>
        /// Invoked on suggestion selection
        /// </summary>
        /// <param name="search">current text</param>
        /// <param name="item">selected suggestion</param>
        /// <returns></returns>
        private string QuerySelector(string search, string item)
        {
            // Get word
            string? word = search.Split(' ').LastOrDefault();
            
            // If none, just add it to the end
            if (string.IsNullOrEmpty(word))
            {
                return search + item;
            }
            
            // Otherwise, remove the last word (suggestion) and emplace
            return search.Remove(search.Length - word.Length) + item;
        }

        /// <summary>
        /// All suggestions
        /// </summary>
        private string[] AutoSource = new[] { "shader:", "message:", "code:" };
        
        /// <summary>
        /// View model helper
        /// </summary>
        private HierarchicalMessageQueryFilterViewModel? VM => DataContext as HierarchicalMessageQueryFilterViewModel;
    }
}