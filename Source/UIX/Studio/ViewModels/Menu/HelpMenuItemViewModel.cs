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

using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Windows.Input;
using Avalonia;
using Avalonia.Media;
using DynamicData;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Documents;

namespace Studio.ViewModels.Menu
{
    public class HelpMenuItemViewModel : ReactiveObject, IMenuItemViewModel
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
        public HelpMenuItemViewModel()
        {
            Items.AddRange(new IMenuItemViewModel[]
            {
                new MenuItemViewModel()
                {
                    Header = "Documentation",
                    Command = ReactiveCommand.Create(OnDocumentation),
                    IconPath = "Question"
                },
                
                new MenuItemViewModel()
                {
                    Header = "What's New",
                    Command = ReactiveCommand.Create(OnWhatsNew),
                    IconPath = "Speaker"
                },
                
                new MenuItemViewModel()
                {
                    Header = "-"
                },
                
                new MenuItemViewModel()
                {
                    Header = "About",
                    Command = ReactiveCommand.Create(OnAbout)
                }
            });
        }

        /// <summary>
        /// Invoked on documentation
        /// </summary>
        private void OnDocumentation()
        {
            // TODO: This is most definitely not the right way, will probably use the built chm or similar instead?
            System.Diagnostics.Process.Start(new ProcessStartInfo
            {
                FileName = "https://github.com/GPUOpen-Tools/GPU-Reshape/blob/main/Documentation/QuickStart.md",
                UseShellExecute = true
            });
        }

        /// <summary>
        /// Invoked on what's new
        /// </summary>
        private void OnWhatsNew()
        {
            if (ServiceRegistry.Get<IWindowService>()?.LayoutViewModel is { } layoutViewModel)
            {
                layoutViewModel.DocumentLayout?.OpenDocument(new WhatsNewDescriptor());
            }
        }

        /// <summary>
        /// Invoked on about
        /// </summary>
        private void OnAbout()
        {
            ServiceRegistry.Get<IWindowService>()?.OpenFor(new AboutViewModel());
        }

        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = "Help";

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = true;
    }
}