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
    public partial class ShaderTreeView : UserControl
    {
        public ShaderTreeView()
        {
            InitializeComponent();

            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Bind signals
                    ShaderList.Events().DoubleTapped
                        .Select(_ => ShaderList.SelectedItem)
                        .WhereNotNull()
                        .InvokeCommand(this, self => self.VM!.OpenShaderDocument);
                });
        }

        /// <summary>
        /// Invoked on layout changes
        /// </summary>
        private void OnListboxLayoutUpdated(object? sender, EventArgs e)
        {
            // Get all the visible items
            ShaderIdentifierViewModel[] visibleElements = ShaderList
                .GetRealizedContainers()
                .Select(x => x.DataContext)
                .Cast<ShaderIdentifierViewModel>()
                .ToArray();

            // Filter out items that are now no longer visible
            if (_lastVisibleElements != null)
            {
                // Linear search, fine for this
                _lastVisibleElements.Where(x => !visibleElements.Contains(x)).ForEach(shaderViewModel =>
                {
                    if (_visibleIdentifiers.Remove(shaderViewModel))
                    {
                        VM?.PooledIdentifiers.Remove(shaderViewModel);
                    }
                });
            }
            
            // Filter in items that are now visible
            foreach (ShaderIdentifierViewModel shaderIdentifierViewModel in visibleElements)
            {
                if (_visibleIdentifiers.Add(shaderIdentifierViewModel))
                {
                    VM?.PooledIdentifiers.Add(shaderIdentifierViewModel);
                }
                
                // If an element just became visible, let's be "responsive" about it
                VM?.PoolImmediate(shaderIdentifierViewModel);
                
                // May already be pooled
                if (!shaderIdentifierViewModel.HasBeenPooled)
                {
                    // Enqueue request
                    VM?.PopulateSingle(shaderIdentifierViewModel);

                    // Mark as pooled
                    shaderIdentifierViewModel.HasBeenPooled = true;
                }
            }
            
            _lastVisibleElements = visibleElements;
        }
        
        /// <summary>
        /// Last visible elements
        /// </summary>
        private ShaderIdentifierViewModel[]? _lastVisibleElements;
        
        /// <summary>
        /// All visible identifiers
        /// </summary>
        private HashSet<ShaderIdentifierViewModel> _visibleIdentifiers = new();

        private ShaderTreeViewModel? VM => DataContext as ShaderTreeViewModel;
    }
}
