using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;

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
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}