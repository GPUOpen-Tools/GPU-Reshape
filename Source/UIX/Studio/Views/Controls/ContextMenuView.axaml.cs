using System;
using System.Collections.Generic;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Metadata;
using ReactiveUI;

namespace Studio.Views.Controls
{
    public partial class ContextMenuView : ContextMenu
    {
        public ContextMenuView()
        {
            InitializeComponent();

            // Set context
            DataContext = new ViewModels.Contexts.ContextViewModel();

            // On placement changed
            this.WhenAnyValue(x => x.PlacementTarget).Subscribe(x =>
            {
                if (DataContext is ViewModels.Contexts.IContextViewModel vm)
                {
                    object? dataViewModel = PlacementTarget?.DataContext;

                    // Expand contained models
                    if (dataViewModel is ViewModels.Controls.IContainedViewModel containedViewModel)
                    {
                        dataViewModel = containedViewModel.ViewModel;
                    }

                    // Set on context menu
                    vm.TargetViewModel = dataViewModel;

                    // Set on children
                    SetViewModels(vm.Items, dataViewModel);
                }
            });
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

                // Cleanup
                e.RoutedEvent?.RouteFinished.Subscribe(_ => _requestedArgs = null);
            }
        }

        private static void SetViewModels(IEnumerable<ViewModels.Contexts.IContextMenuItem> items, object? viewModel)
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