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
using System.Linq;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.VisualTree;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace.Message;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.Views.Workspace.Properties
{
    public partial class MessageCollectionView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel { get => DataContext; set => DataContext = value; }
        
        public MessageCollectionView()
        {
            InitializeComponent();
            
            // On data context
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<MessageCollectionViewModel>()
                .Subscribe(viewModel =>
                {
                    // Set query filter
                    CollectionQueryView.DataContext = viewModel.HierarchicalMessageQueryFilterViewModel;
                    
                    // Bind signals
                    MessageList.AddHandler(InputElement.PointerPressedEvent, OnListPointerPressed, RoutingStrategies.Tunnel);
                });
            
            
            // Bind activation
            this.WhenActivated(_ =>
            {
                if (DataContext is not MessageCollectionViewModel vm)
                {
                    return;
                }
                
                // Reconstruct and bind scroll events
#if false
                if (MessageTree.GetVisualDescendants().OfType<ScrollViewer>().FirstOrDefault() is { } scrollViewer)
                {
                    scrollViewer.Offset = vm.ScrollAmount;
                    scrollViewer.Events().ScrollChanged.Subscribe(x => vm.ScrollAmount = scrollViewer.Offset);
                }
#endif
            });
        }

        /// <summary>
        /// Invoked on pointer presses
        /// </summary>
        private void OnListPointerPressed(object? sender, PointerPressedEventArgs _event)
        {
            // Get contexts
            if (DataContext is not MessageCollectionViewModel {} viewModel ||
                MessageList.SelectedItem is not IObservableTreeItem { } item)
            {
                return;
            }

            // Double click?
            if (_event.ClickCount < 2)
            {
                return;
            }

            // Open document for item
            viewModel.OpenShaderDocument.Execute(item);
            _event.Handled = true;
        }
    }
}