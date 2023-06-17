// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
using Avalonia.Media;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;

namespace Studio.ViewModels.Controls
{
    public interface IObservableTreeItem
    {
        /// <summary>
        /// Display text of this item
        /// </summary>
        public string Text { get; set; }
        
        /// <summary>
        /// Expanded state of this item
        /// </summary>
        public bool IsExpanded { get; set; }

        /// <summary>
        /// Associated view model contained within this tree item
        /// </summary>
        public object? ViewModel { get; set; }
        
        /// <summary>
        /// Foreground color of the item
        /// </summary>
        public ISolidColorBrush? StatusColor { get; set; }

        /// <summary>
        /// All child items
        /// </summary>
        public ObservableCollection<IObservableTreeItem> Items { get; }
    }
}