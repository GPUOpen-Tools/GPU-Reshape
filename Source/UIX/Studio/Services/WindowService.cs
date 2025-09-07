// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

using System.Threading.Tasks;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Runtime.ViewModels;
using Studio.ViewModels;
using Studio.Views;

namespace Studio.Services
{
    public class WindowService : IWindowService
    {
        /// <summary>
        /// Shared layout
        /// </summary>
        public ILayoutViewModel? LayoutViewModel { get; set; } = null;

        /// <summary>
        /// Open a dialog for a given view model
        /// </summary>
        /// <param name="viewModel">view model</param>
        public Task<IWindowViewModel?> OpenDialogFor(object viewModel)
        {
            // Must have desktop lifetime
            if (Application.Current?.ApplicationLifetime is not IClassicDesktopStyleApplicationLifetime desktop)
            {
                return Task.FromResult<IWindowViewModel?>(null);
            }

            // Attempt to resolve
            // Try window before primary
            Window? window = _locator?.InstantiateDerived<Window>(viewModel, ViewType.Window) ?? _locator?.InstantiateDerived<Window>(viewModel);
            if (window == null)
            {
                return Task.FromResult<IWindowViewModel?>(null);
            }

            // Properties
            window.WindowStartupLocation = WindowStartupLocation.CenterOwner;
            window.DataContext = viewModel;

            // Show dialog as task
            return window.ShowDialog(desktop.MainWindow).ContinueWith<IWindowViewModel?>(_ => new WindowViewModel
            {
                Window = window
            });
        }

        /// <summary>
        /// Open a window for a given view model
        /// </summary>
        /// <param name="viewModel">view model</param>
        public IWindowViewModel? OpenFor(object viewModel)
        {
            // Must have desktop lifetime
            if (Application.Current?.ApplicationLifetime is not IClassicDesktopStyleApplicationLifetime desktop)
            {
                return null;
            }

            // Attempt to resolve
            // Try window before primary
            Window? window = _locator?.InstantiateDerived<Window>(viewModel, ViewType.Window) ?? _locator?.InstantiateDerived<Window>(viewModel);
            if (window == null)
            {
                return null;
            }

            // Properties
            window.WindowStartupLocation = WindowStartupLocation.CenterOwner;
            window.DataContext = viewModel;
            
            // Show it
            window.Show(desktop.MainWindow);

            // Show dialog as task
            return new WindowViewModel
            {
                Window = window
            };
        }

        /// <summary>
        /// Request exit
        /// </summary>
        public void Exit()
        {
            if (Application.Current?.ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
            {
                desktop.MainWindow.Close();
            }
        }

        /// <summary>
        /// Locator service
        /// </summary>
        private ILocatorService? _locator = ServiceRegistry.Get<ILocatorService>();
    }
}