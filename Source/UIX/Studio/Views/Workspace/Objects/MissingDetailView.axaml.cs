using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;

namespace Studio.Views.Workspace.Objects
{
    public partial class MissingDetailView : UserControl, IViewFor
    {
        public object? ViewModel
        {
            get => DataContext;
            set => DataContext = value;
        }

        public MissingDetailView()
        {
            InitializeComponent();
        }
    }
}