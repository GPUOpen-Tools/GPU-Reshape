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
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Metadata;
using ReactiveUI;
using Studio.Extensions;
using Studio.Services;
using Studio.ViewModels.Contexts;

namespace Studio.Views.Controls
{
    public partial class ContextMenuView : ContextMenu
    {
        public ContextMenuView()
        {
            InitializeComponent();
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
                if (ServiceRegistry.Get<IContextMenuService>() is { } service)
                {
                    IEnumerable<object>? dataViewModelSource = null;

                    // Determine appropriate view model
                    if (FindParent<TreeViewItem>(e.Source as Control) is {} treeViewItem)
                    {
                        if (FindParent<TreeView>(treeViewItem) is {} parentTree)
                        {
                            dataViewModelSource = parentTree.SelectedItems.Cast<object>();
                        }
                    }
                    else if (FindParent<ListBoxItem>(e.Source as Control) is {} listBoxItem)
                    {
                        if (FindParent<ListBox>(listBoxItem) is {} parentList)
                        {
                            dataViewModelSource = parentList.SelectedItems?.Cast<object>() ?? Enumerable.Empty<object>();
                        }
                    }
                    
                    // If none found, just assume the immediate data context
                    if (dataViewModelSource == null)
                    {
                        dataViewModelSource = control.DataContext.SingleEnumerable();
                    }
                    
                    // Unwrap all view models
                    object[]? dataViewModels = dataViewModelSource?.Select(dataViewModel =>
                    {
                        if (dataViewModel is ViewModels.Controls.IContainedViewModel containedViewModel)
                        {
                            return containedViewModel.ViewModel;
                        }

                        // No layering
                        return dataViewModel;
                    }).OfType<object>().ToArray();

                    // Dynamically populated items
                    List<IContextMenuItemViewModel> items = new();

                    // Install all service models
                    if (dataViewModels != null)
                    {
                        foreach (IContextViewModel serviceViewModel in service.ViewModels)
                        {
                            serviceViewModel.Install(items, dataViewModels);
                        }
                    }

                    // If no services, just close it
                    if (items.Count == 0)
                    {
                        control.ContextMenu.Close();
                        return;
                    }
                    
                    // Setup all items
                    foreach (IContextMenuItemViewModel contextMenuItemViewModel in items)
                    {
                        // Set on context menu
                        contextMenuItemViewModel.TargetViewModels = dataViewModels;

                        // Set on children
                        SetViewModels(contextMenuItemViewModel.Items, dataViewModels);
                    }

                    // Assign to control
                    control.ContextMenu.ItemsSource = items;
                }

                // Cleanup
                e.RoutedEvent?.RouteFinished.Subscribe(_ => _requestedArgs = null);
            }
        }

        private static void SetViewModels(IEnumerable<ViewModels.Contexts.IContextMenuItemViewModel> items, object[]? viewModels)
        {
            // Set on children
            foreach (var item in items)
            {
                item.TargetViewModels = viewModels;
                SetViewModels(item.Items, viewModels);
            }
        }

        private static T? FindParent<T>(Control? control) where T : class
        {
            while (control != null)
            {
                if (control is T typed)
                {
                    return typed;
                }

                control = control.Parent as Control;
            }

            return null;
        }

        /// <summary>
        /// Internally tracked event stack
        /// </summary>
        private static ContextRequestedEventArgs? _requestedArgs = null;
    }
}