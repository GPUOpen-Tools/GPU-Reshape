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

using System;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using AvaloniaEdit.TextMate;
using AvaloniaEdit.Utils;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;
using Studio.Views.Editor;
using TextMateSharp.Grammars;
using ShaderViewModel = Studio.ViewModels.Documents.ShaderViewModel;

namespace Studio.Views.Shader
{
    public partial class ILShaderContentView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel { get => DataContext; set => DataContext = value; }
        
        public ILShaderContentView()
        {
            InitializeComponent();

            // Create the registry
            var registryOptions = new TextMate.RegistryOptions();

            // Create a text mate instance
            var textMate = Editor.InstallTextMate(registryOptions);

            // Set the default grammar (just assume .hlsl)
            textMate.SetGrammar("IL");

            // Common styling
            Editor.Options.IndentationSize = 4;
            
            // Bind contents
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<ILShaderContentViewModel>()
                .Subscribe(viewModel =>
            {
                viewModel.WhenAnyValue(x => x.Object).WhereNotNull().Subscribe(x =>
                {
                    x.WhenAnyValue(y => y.IL).Subscribe(contents =>
                    {
                        Editor.Text = contents;
                    });
                });
            });
        }
    }
}