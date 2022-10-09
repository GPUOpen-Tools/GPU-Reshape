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
    public partial class FeatureCollectionView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel { get => DataContext; set => DataContext = value; }
        
        public FeatureCollectionView()
        {
            InitializeComponent();
        }
    }
}