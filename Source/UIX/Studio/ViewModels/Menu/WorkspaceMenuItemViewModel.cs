// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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

using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using DynamicData;
using ReactiveUI;
using Studio.Services;

namespace Studio.ViewModels.Menu
{
    public class WorkspaceMenuItemViewModel : ReactiveObject, IMenuItemViewModel
    {
        /// <summary>
        /// Given header
        /// </summary>
        public string Header
        {
            get => _header;
            set => this.RaiseAndSetIfChanged(ref _header, value);
        }

        /// <summary>
        /// Is enabled?
        /// </summary>
        public bool IsEnabled
        {
            get => _isEnabled;
            set => this.RaiseAndSetIfChanged(ref _isEnabled, value);
        }

        /// <summary>
        /// Items within
        /// </summary>
        public ObservableCollection<IMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Target command
        /// </summary>
        public ICommand? Command { get; } = null;

        /// <summary>
        /// Icon for this item
        /// </summary>
        public StreamGeometry? Icon => null;

        /// <summary>
        /// Constructor
        /// </summary>
        public WorkspaceMenuItemViewModel()
        {
            Items.AddRange(new IMenuItemViewModel[]
            {
                new MenuItemViewModel()
                {
                    Header = "Launch Application",
                    Command = ReactiveCommand.Create(OnLaunchApplication),
                    IconPath = "OpenInApp"
                },
                
                new MenuItemViewModel()
                {
                    Header = "-"
                },
                
                new MenuItemViewModel()
                {
                    Header = "Create Connection",
                    Command = ReactiveCommand.Create(OnCreateConnection),
                    IconPath = "Connect"
                },
                
                new MenuItemViewModel()
                {
                    Header = "Close All Connections",
                    Command = ReactiveCommand.Create(OnCloseAllConnections)
                }
            });
        }

        /// <summary>
        /// Invoked on application launches
        /// </summary>
        private void OnLaunchApplication()
        {
            App.Locator.GetService<IWindowService>()?.OpenFor(new LaunchViewModel());
        }

        /// <summary>
        /// Invoked on connection creation
        /// </summary>
        private void OnCreateConnection()
        {
            App.Locator.GetService<IWindowService>()?.OpenFor(new ConnectViewModel());
        }

        /// <summary>
        /// Invoked on close connections
        /// </summary>
        private void OnCloseAllConnections()
        {
            App.Locator.GetService<IWorkspaceService>()?.Clear();
        }

        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = "Workspace";

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = true;
    }
}