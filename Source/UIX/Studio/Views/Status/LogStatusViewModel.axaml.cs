using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;

namespace Studio.Views.Status
{
    public partial class LogStatusView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel { get => DataContext; set => DataContext = value; }
        
        public LogStatusView()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}