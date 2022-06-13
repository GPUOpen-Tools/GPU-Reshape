using System.Reactive;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using DynamicData.Binding;
using ReactiveUI;
using Studio.Extensions;

namespace Studio.Views
{
    public partial class ConnectWindow : Window
    {
        public ConnectWindow()
        {
            InitializeComponent();
#if DEBUG
            this.AttachDevTools();
#endif

            // Bind interactions
            VM?.AcceptClient.RegisterHandler(ctx => {
                // Request closure
                Close(true);
            
                // OK
                ctx.SetOutput(true);
            });

            // Bind events
            ConnectionGrid.Events().DoubleTapped
                .ToSignal()
                .InvokeCommand(VM, vm => vm.Connect);
        }
        
        private ViewModels.ConnectViewModel? VM => DataContext as ViewModels.ConnectViewModel;
    }
}