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

using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Reactive;
using System.Windows.Input;
using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.Extensions;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Contexts
{
    public class InstrumentNoneContextViewModel : ReactiveObject, IContextMenuItemViewModel
    {
        /// <summary>
        /// Target view models of the context
        /// </summary>
        public object[]? TargetViewModels
        {
            get => _targetViewModels;
            set
            {
                this.RaiseAndSetIfChanged(ref _targetViewModels, value);
                IsVisible = _targetViewModels?.Any(x => x is IInstrumentableObject) ?? false;
            }
        }

        /// <summary>
        /// Display header of this context model
        /// </summary>
        public string Header { get; set; } = "None";
        
        /// <summary>
        /// All items within this context model
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Target command
        /// </summary>
        public ICommand Command { get; }

        /// <summary>
        /// Is this context enabled?
        /// </summary>
        public bool IsVisible
        {
            get => _isEnabled;
            set => this.RaiseAndSetIfChanged(ref _isEnabled, value);
        }

        public InstrumentNoneContextViewModel()
        {
            Command = ReactiveCommand.Create(OnInvoked);
        }

        /// <summary>
        /// Command implementation
        /// </summary>
        private void OnInvoked()
        {
            foreach (IPropertyViewModel propertyViewModel in _targetViewModels?.Promote<IPropertyViewModel>() ?? Array.Empty<IPropertyViewModel>())
            {
                Traverse(propertyViewModel);
            }
        }

        /// <summary>
        /// Traverse an item for closing
        /// </summary>
        private void Traverse(IPropertyViewModel propertyViewModel)
        {
            // Is instrumentable object and closable?
            // Exception for the actual workspace, never close that
            if (propertyViewModel is IInstrumentableObject and IClosableObject closableObject and not IWorkspaceAdapter)
            {
                closableObject.CloseCommand?.Execute(Unit.Default);
                return;
            }
            
            // Not applicable, check child properties
            propertyViewModel.Properties.Items.ForEach(Traverse);
        }

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isEnabled = false;

        /// <summary>
        /// Internal target view models
        /// </summary>
        private object[]? _targetViewModels;
    }
}