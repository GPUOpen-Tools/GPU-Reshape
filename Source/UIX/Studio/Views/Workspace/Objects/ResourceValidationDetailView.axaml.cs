using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;

namespace Studio.Views.Workspace.Objects
{
    public partial class ResourceValidationDetailView : UserControl, IViewFor
    {
        public object? ViewModel
        {
            get => DataContext;
            set => DataContext = value;
        }

        public ResourceValidationDetailView()
        {
            InitializeComponent();
        }
    }
}