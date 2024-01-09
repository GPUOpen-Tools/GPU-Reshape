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
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Metadata;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Contexts;

namespace Studio.Views.Controls
{
    public partial class ContextMenuView : ContextMenu
    {
        public ContextMenuView()
        {
            InitializeComponent();

            // Set context
            DataContext = App.Locator.GetService<IContextMenuService>()?.ViewModel;
        }

        static ContextMenuView()
        {
            // Listen to all ContextMenu properties, subscribe to their context requests
            ContextMenuProperty.Changed.Subscribe(e =>
            {
                var control = (Control)e.Sender;

                if (e.OldValue.Value is ContextMenu)
                {
                    control.RemoveHandler(ContextRequestedEvent, OnContextRequested);
                }

                if (e.NewValue.Value is ContextMenu)
                {
                    // Filter in handled events as the default context handler will mark as accepted
                    control.AddHandler(ContextRequestedEvent, OnContextRequested, handledEventsToo: true);
                }
            });
        }

        private static void OnContextRequested(object? sender, ContextRequestedEventArgs e)
        {
            // Valid target?
            if (sender is not Control control || control.ContextMenu == null)
            {
                return;
            }

            // Top of stack?
            //   ? Due to handling accepted events we must manage the stack manually
            if (!ReferenceEquals(_requestedArgs, e))
            {
                _requestedArgs = e;

                // Set placement
                control.ContextMenu.PlacementTarget = control;

                // Get service
                if (App.Locator.GetService<IContextMenuService>()?.ViewModel is { } contextMenuItemViewModel)
                {
                    // Determine appropriate view model
                    object? dataViewModel;
                    if (control is ListBox listbox)
                    {
                        dataViewModel = listbox.SelectedItem;
                    }
                    else
                    {
                        dataViewModel = control.DataContext;
                    }

                    // Expand contained models
                    if (dataViewModel is ViewModels.Controls.IContainedViewModel containedViewModel)
                    {
                        dataViewModel = containedViewModel.ViewModel;
                    }

                    // Set on context menu
                    contextMenuItemViewModel.TargetViewModel = dataViewModel;

                    // Set on children
                    SetViewModels(contextMenuItemViewModel.Items, dataViewModel);
                }

                // Cleanup
                e.RoutedEvent?.RouteFinished.Subscribe(_ => _requestedArgs = null);
            }
        }

        private static void SetViewModels(IEnumerable<ViewModels.Contexts.IContextMenuItemViewModel> items, object? viewModel)
        {
            // Set on children
            foreach (var item in items)
            {
                item.TargetViewModel = viewModel;
                SetViewModels(item.Items, viewModel);
            }
        }

        /// <summary>
        /// Internally tracked event stack
        /// </summary>
        private static ContextRequestedEventArgs? _requestedArgs = null;
    }
}