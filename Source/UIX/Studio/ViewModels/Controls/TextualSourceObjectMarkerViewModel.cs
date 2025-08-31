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
using System.Windows.Input;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Controls
{
    public class TextualSourceObjectMarkerViewModel : ReactiveObject
    {
        /// <summary>
        /// Transformed source line
        /// </summary>
        public int SourceLine { get; set; }

        /// <summary>
        /// The textual view model
        /// </summary>
        public ITextualShaderContentViewModel? ShaderContentViewModel
        {
            get => _shaderContentViewModel;
            set => this.RaiseAndSetIfChanged(ref _shaderContentViewModel, value);
        }

        /// <summary>
        /// The command to be invoked on details
        /// </summary>
        public ICommand? DetailCommand
        {
            get => _detailCommand;
            set => this.RaiseAndSetIfChanged(ref _detailCommand, value);
        }
        
        public ObservableCollection<TextualSourceObjectMarkerCategoryViewModel> CategoryObjects { get; } = new();

        /// <summary>
        /// Internal detail state
        /// </summary>
        private ICommand? _detailCommand;

        /// <summary>
        /// Internal content view model
        /// </summary>
        private ITextualShaderContentViewModel? _shaderContentViewModel;
    }
}