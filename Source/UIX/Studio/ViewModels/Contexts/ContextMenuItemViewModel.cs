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

namespace Studio.ViewModels.Contexts
{
    public class ContextMenuItemViewModel : IContextMenuItemViewModel
    {
        /// <summary>
        /// Given header
        /// </summary>
        public string Header { get; set; } = string.Empty;

        /// <summary>
        /// All hosted items
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; } = new();

        /// <summary>
        /// Command on invoke
        /// </summary>
        public ICommand? Command { get; } = null;

        /// <summary>
        /// If this context menu is enabled
        /// </summary>
        public bool IsVisible { get; set; } = true;
        
        /// <summary>
        /// Underlying target view model for the context menu
        /// </summary>
        public object? TargetViewModel { get; set; }
    }
}