using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Traits;

namespace Studio.Views.Controls
{
    public partial class FileTreeItemView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel { get => DataContext; set => DataContext = value; }
        
        public FileTreeItemView()
        {
            InitializeComponent();
        }
    }
}