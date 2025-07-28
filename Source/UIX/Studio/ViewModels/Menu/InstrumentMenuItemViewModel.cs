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
using System.Windows.Input;
using System;
using System.Collections.Generic;
using Avalonia.Media;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Contexts;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Menu
{
    public class InstrumentMenuItemViewModel : ReactiveObject, IMenuItemViewModel
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
        /// Is visible?
        /// </summary>
        public bool IsVisible
        {
            get => _isVisible;
            set => this.RaiseAndSetIfChanged(ref _isVisible, value);
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
        public InstrumentMenuItemViewModel()
        {
            if (ServiceRegistry.Get<IWorkspaceService>() is { } service)
            {
                service.WhenAnyValue(x => x.SelectedWorkspace).Subscribe(OnWorkspaceChanged);
            }
        }

        /// <summary>
        /// Invoked on workspace changes
        /// </summary>
        private void OnWorkspaceChanged(IWorkspaceViewModel? workspace)
        {
            Items.Clear();

            // If not instrumentable, skip
            IsVisible = workspace is { PropertyCollection: IInstrumentableObject };
            if (!IsVisible)
            {
                return;
            }
            
            // Get instrumentation context from workspace
            List<IContextMenuItemViewModel> itemContexts = new();
            _context.Install(itemContexts, new object []{ workspace!.PropertyCollection });

            // Populate contexts
            if (itemContexts.Count > 0)
            {
                InstallContextViewModels(this, itemContexts[0].Items);
            }
        }

        /// <summary>
        /// Install all context items
        /// </summary>
        private void InstallContextViewModels(IMenuItemViewModel viewModel, IList<IContextMenuItemViewModel> itemViewModels)
        {
            foreach (IContextMenuItemViewModel itemViewModel in itemViewModels)
            {
                // May not be visible
                if (!itemViewModel.IsVisible)
                {
                    continue;
                }

                MenuItemViewModel menuItem = new()
                {
                    Header = itemViewModel.Header,
                    Command = itemViewModel.Command
                };
                viewModel.Items.Add(menuItem);
                
                InstallContextViewModels(menuItem, itemViewModel.Items);
            }
        }
        
        /// <summary>
        /// Shared instrumentation context
        /// </summary>
        private InstrumentContextViewModel _context = new();
        
        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = "Instrument";

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = true;

        /// <summary>
        /// Internal visible state
        /// </summary>
        private bool _isVisible = true;
    }
}