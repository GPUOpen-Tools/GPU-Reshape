using System;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using DynamicData;
using DynamicData.Binding;
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
                .CastNullable<MessageCollectionViewModel>()
                .Subscribe(viewModel =>
                {
                    // Update arrangement
                    viewModel.ValidationObjects.ToObservableChangeSet()
                        .OnItemAdded(x => this.InvalidateArrange())
                        .Subscribe();
                    
                    // Bind signals
                    MessageDataGrid.Events().DoubleTapped
                        .Select(_ => MessageDataGrid.SelectedItem as ValidationObject)
                        .WhereNotNull()
                        .Subscribe(x => viewModel.OpenShaderDocument.Execute(x));
                });
        }
    }
}