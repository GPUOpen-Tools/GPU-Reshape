using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Dock.Avalonia;
using DynamicData;
using Studio.Services;

namespace Studio.Views
{
    public partial class MainView : UserControl
    {
        public MainView()
        {
            InitializeComponent();

            InitializeMenu();
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }

        private void InitializeMenu()
        {
            
        }
    }
}