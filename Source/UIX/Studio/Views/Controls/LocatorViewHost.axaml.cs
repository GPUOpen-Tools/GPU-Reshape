using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.ViewModels.Controls;

namespace Studio.Views.Controls
{
    public partial class LocatorViewHost : UserControl
    {
        public LocatorViewHost()
        {
            InitializeComponent();

            // Set locator
            ViewHost.ViewLocator = new ViewLocator();
        }
    }
}