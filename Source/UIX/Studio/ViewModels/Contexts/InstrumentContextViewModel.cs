// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
using System.Reactive;
using System.Windows.Input;
using Message.CLR;
using ReactiveUI;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Contexts
{
    public class InstrumentContextViewModel : ReactiveObject, IInstrumentContextViewModel
    {
        /// <summary>
        /// Target view model of the context
        /// </summary>
        public object? TargetViewModel
        {
            get => _targetViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _targetViewModel, value);
                IsVisible = _targetViewModel is IInstrumentableObject;
            }
        }

        /// <summary>
        /// Display header of this context model
        /// </summary>
        public string Header { get; set; } = "Instrument";
        
        /// <summary>
        /// All items within this context model
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Target command
        /// </summary>
        public ICommand? Command { get; } = null;

        public InstrumentContextViewModel()
        {
            // Standard objects
            Items.Add(new InstrumentNoneContextViewModel());
            Items.Add(new InstrumentAllContextViewModel());
        }

        /// <summary>
        /// Is this context enabled?
        /// </summary>
        public bool IsVisible
        {
            get => _isVisible;
            set => this.RaiseAndSetIfChanged(ref _isVisible, value);
        }

        /// <summary>
        /// Internal enabled state
        /// </summary>
        private bool _isVisible = false;

        /// <summary>
        /// Internal target view model
        /// </summary>
        private object? _targetViewModel;
    }
}