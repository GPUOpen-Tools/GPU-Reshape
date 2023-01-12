using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.ViewModels.Status;

namespace Studio.Views.Status
{
    public partial class NetworkStatusView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel { get => DataContext; set => DataContext = value; }
        
        public NetworkStatusView()
        {
            InitializeComponent();
        }
    }
}