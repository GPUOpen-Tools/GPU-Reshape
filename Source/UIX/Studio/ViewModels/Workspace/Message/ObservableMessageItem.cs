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
using Avalonia.Media;
using ReactiveUI;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace.Objects;
using ShaderViewModel = Studio.ViewModels.Workspace.Objects.ShaderViewModel;

namespace Studio.ViewModels.Workspace.Message
{
    public class ObservableMessageItem : ReactiveObject, IObservableTreeItem
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
        public IBrush? StatusColor
        {
            get => _statusColor;
            set => this.RaiseAndSetIfChanged(ref _statusColor, value);
        }
        
        /// <summary>
        /// Hosted view model
        /// </summary>
        public object? ViewModel
        {
            get => _validationObject;
            set
            {
                var validationObject = value as ValidationObject ?? throw new NotSupportedException("Invalid view model");
                this.RaiseAndSetIfChanged(ref _validationObject, validationObject);
            }
        }

        /// <summary>
        /// Validation object this observable reflects
        /// </summary>
        public ValidationObject? ValidationObject
        {
            get => _validationObject;
            set => this.RaiseAndSetIfChanged(ref _validationObject, value);
        }

        /// <summary>
        /// Resolved shader of the validation object
        /// </summary>
        public ShaderViewModel? ShaderViewModel
        {
            get { return _shaderViewModel; }
            set { this.RaiseAndSetIfChanged(ref _shaderViewModel, value); }
        }

        /// <summary>
        /// The decorated extract
        /// </summary>
        public string ExtractDecoration
        {
            get => _extractDecoration;
            set => this.RaiseAndSetIfChanged(ref _extractDecoration, value);
        }

        /// <summary>
        /// The decorated filename
        /// </summary>
        public string FilenameDecoration
        {
            get => _filenameDecoration;
            set => this.RaiseAndSetIfChanged(ref _filenameDecoration, value);
        }

        /// <summary>
        /// All child items
        /// </summary>
        public ObservableCollection<IObservableTreeItem> Items { get; } = new();

        /// <summary>
        /// Internal connection state
        /// </summary>
        private ValidationObject? _validationObject;

        /// <summary>
        /// Internal text state
        /// </summary>
        private string _text = "ObservableTreeItem";

        /// <summary>
        /// Internal status color
        /// </summary>
        private IBrush? _statusColor = Brushes.White;

        /// <summary>
        /// Internal shader view model
        /// </summary>
        private ShaderViewModel? _shaderViewModel;

        /// <summary>
        /// Internal extract
        /// </summary>
        private string _extractDecoration = string.Empty;

        /// <summary>
        /// Internal filename
        /// </summary>
        private string _filenameDecoration = string.Empty;

        /// <summary>
        /// Internal expansion state
        /// </summary>
        private bool _isExpanded = true;
    }
}