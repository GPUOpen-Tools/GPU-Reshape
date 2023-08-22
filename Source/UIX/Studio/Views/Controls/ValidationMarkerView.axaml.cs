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
using Avalonia.Controls;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Controls;

namespace Studio.Views.Controls
{
    public partial class ValidationMarkerView : UserControl
    {
        /// <summary>
        /// View model helper
        /// </summary>
        public ValidationMarkerViewModel ViewModel => (ValidationMarkerViewModel)DataContext!;
        
        public ValidationMarkerView()
        {
            InitializeComponent();

            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<ValidationMarkerViewModel>()
                .Subscribe(x =>
                {
                    // Bind detail click
                    ValidationButton.Events().Click.Subscribe(_ =>
                    {
                        x.DetailCommand?.Execute(x.SelectedObject);
                    });
                    
                    // Hide flyout on selection
                    x.WhenAnyValue(y => y.SelectedObject).Subscribe(_ =>
                    {
                        DropdownButton.Flyout.Hide();
                    });
                });
        }
    }
}