using System;
using System.Reactive.Linq;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using DynamicData;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Tools;

namespace Studio.Views.Tools
{
    public partial class LogView : UserControl
    {
        public LogView()
        {
            InitializeComponent();

            // Bind context
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<LogViewModel>()
                .Subscribe(viewModel =>
                {
                    // Bind events to unfiltered collection
                    viewModel.LoggingViewModel?.Events
                        .Connect()
                        .OnItemAdded(x =>
                        {
                            // Check if scroll locked
                            if (viewModel.IsScrollLock && LogListbox.ItemCount > 0)
                            {
                                LogListbox.ScrollIntoView(LogListbox.ItemCount - 1);
                            }
                        })
                        .Subscribe();

                    // Bind scroll lock event
                    viewModel.WhenAnyValue(x => x.IsScrollLock).Subscribe(x =>
                    {
                        if (x && LogListbox.ItemCount > 0)
                        {
                            LogListbox.ScrollIntoView(LogListbox.ItemCount - 1);
                        }
                    });
                });

            // Disable lock on scroll
            LogListbox.Events().PointerWheelChanged.Subscribe(x =>
            {
                if (DataContext is LogViewModel vm)
                {
                    vm.IsScrollLock = false;
                }
            });
        }
    }
}