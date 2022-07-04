using System;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Workspace.Objects;
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

            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Bind signals
                    MessageDataGrid.Events().DoubleTapped
                        .Select(_ => MessageDataGrid.SelectedItem as ValidationObject)
                        .WhereNotNull()
                        .InvokeCommand(this, x => x._viewModel!.OpenShaderDocument);
                });
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private MessageCollectionViewModel? _viewModel => ViewModel as MessageCollectionViewModel;
    }
}