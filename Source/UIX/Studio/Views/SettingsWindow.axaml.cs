using System;
using System.Reactive;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using DynamicData.Binding;
using ReactiveUI;
using Studio.Extensions;

namespace Studio.Views
{
    public partial class SettingsWindow : Window
    {
        public SettingsWindow()
        {
            InitializeComponent();
#if DEBUG
            this.AttachDevTools();
#endif
        }
        
        private ViewModels.SettingsViewModel? VM => DataContext as ViewModels.SettingsViewModel;
    }
}