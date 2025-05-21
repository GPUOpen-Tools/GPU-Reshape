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
using System.Linq;
using System.Reactive.Linq;
using Avalonia.Controls;
using ReactiveUI;
using Runtime.ViewModels.Objects;
using Studio.Extensions;
using Studio.ViewModels.Tools;

namespace Studio.Views.Tools
{
    public partial class PipelineTreeView : UserControl
    {
        public PipelineTreeView()
        {
            InitializeComponent();

            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Bind signals
                    PipelineList.Events().DoubleTapped
                        .Select(_ => PipelineList.SelectedItem)
                        .WhereNotNull()
                        .InvokeCommand(this, self => self.VM!.OpenPipelineDocument);
                });
        }

        /// <summary>
        /// Invoked on layout changes
        /// </summary>
        private void OnListboxLayoutUpdated(object? sender, EventArgs e)
        {
            // Get all the visible items
            PipelineIdentifierViewModel[] visibleElements = PipelineList
                .GetRealizedContainers()
                .Select(x => x.DataContext)
                .Cast<PipelineIdentifierViewModel>()
                .ToArray();

            // Filter out items that are now no longer visible
            if (_lastVisibleElements != null)
            {
                // Linear search, fine for this
                _lastVisibleElements.Where(x => !visibleElements.Contains(x)).ForEach(pipelineViewModel =>
                {
                    if (_visibleIdentifiers.Remove(pipelineViewModel))
                    {
                        VM?.PooledIdentifiers.Remove(pipelineViewModel);
                    }
                });
            }
            
            // Filter in items that are now visible
            foreach (PipelineIdentifierViewModel pipelineIdentifierViewModel in visibleElements)
            {
                if (_visibleIdentifiers.Add(pipelineIdentifierViewModel))
                {
                    VM?.PooledIdentifiers.Add(pipelineIdentifierViewModel);
                }
                
                // If an element just became visible, let's be "responsive" about it
                VM?.PoolImmediate(pipelineIdentifierViewModel);
                
                // May already be pooled
                if (!pipelineIdentifierViewModel.HasBeenPooled)
                {
                    // Enqueue request
                    VM?.PopulateSingle(pipelineIdentifierViewModel);

                    // Mark as pooled
                    pipelineIdentifierViewModel.HasBeenPooled = true;
                }
            }
            
            _lastVisibleElements = visibleElements;
        }

        /// <summary>
        /// Last visible elements
        /// </summary>
        private PipelineIdentifierViewModel[]? _lastVisibleElements;
        
        /// <summary>
        /// All visible identifiers
        /// </summary>
        private HashSet<PipelineIdentifierViewModel> _visibleIdentifiers = new();

        private PipelineTreeViewModel? VM => DataContext as PipelineTreeViewModel;
    }
}
