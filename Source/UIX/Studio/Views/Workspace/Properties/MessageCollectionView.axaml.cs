using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Extensions;
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

            MessageDataGrid.Events().DoubleTapped.ToSignal();
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private MessageCollectionViewModel? _viewModel => ViewModel as MessageCollectionViewModel;
    }
}