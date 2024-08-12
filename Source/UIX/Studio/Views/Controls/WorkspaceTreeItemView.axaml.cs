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
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Traits;

namespace Studio.Views.Controls
{
    public partial class WorkspaceTreeItemView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel { get => DataContext; set => DataContext = value; }
        
        public WorkspaceTreeItemView()
        {
            InitializeComponent();

            // Bind on context
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<WorkspaceTreeItemViewModel>()
                .Subscribe(itemViewModel =>
                {
                    // React to instrumentation changes
                    if (itemViewModel.ViewModel is IInstrumentableObject instrumentableObject)
                    {
                        instrumentableObject.WhenAnyValue(x => x.InstrumentationState).Subscribe(_ =>
                        {
                            itemViewModel.RaisePropertyChanged(nameof(itemViewModel.ViewModel));
                        });
                    }
                    
                    // Bind signals
                    this.Events().DoubleTapped.Subscribe(x =>
                    {
                        x.Handled = true;
                        itemViewModel.OpenDocument.Execute(null);
                    });
                });
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}