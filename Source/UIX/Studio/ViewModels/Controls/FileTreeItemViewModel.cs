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

using System;
using System.Collections.ObjectModel;
using Avalonia.Media;
using ReactiveUI;
using Runtime.ViewModels.Shader;

namespace Studio.ViewModels.Controls
{
    public class FileTreeItemViewModel : ReactiveObject, IObservableTreeItem
    {
        /// <summary>
        /// Display text of this item
        /// </summary>
        public string Text
        {
            get { return _text; }
            set { this.RaiseAndSetIfChanged(ref _text, value); }
        }
        
        /// <summary>
        /// Hosted view model
        /// </summary>
        /// <exception cref="NotSupportedException"></exception>
        public object? ViewModel
        {
            get => _shaderFileViewModel;
            set => this.RaiseAndSetIfChanged(ref _shaderFileViewModel, value as ShaderFileViewModel);
        }

        /// <summary>
        /// Expansion state
        /// </summary>
        public bool IsExpanded
        {
            get { return _isExpanded; }
            set { this.RaiseAndSetIfChanged(ref _isExpanded, value); }
        }

        /// <summary>
        /// Foreground color of the item
        /// </summary>
        public ISolidColorBrush? StatusColor
        {
            get => _statusColor;
            set => this.RaiseAndSetIfChanged(ref _statusColor, value);
        }
        
        /// <summary>
        /// All child items
        /// </summary>
        public ObservableCollection<IObservableTreeItem> Items { get; } = new();

        /// <summary>
        /// Internal text state
        /// </summary>
        private string _text = "FileTreeItemViewModel";

        /// <summary>
        /// Internal status color
        /// </summary>
        private ISolidColorBrush? _statusColor = Brushes.White;

        /// <summary>
        /// Internal file
        /// </summary>
        private ShaderFileViewModel? _shaderFileViewModel;
        
        /// <summary>
        /// Internal expansion state
        /// </summary>
        private bool _isExpanded = true;
    }
}