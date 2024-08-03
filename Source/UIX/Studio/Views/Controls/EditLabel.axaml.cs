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
using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Studio.Extensions;

namespace Studio.Views.Controls
{
    public partial class EditLabel : UserControl
    {
        /// <summary>
        /// Text property
        /// </summary>
        public static readonly StyledProperty<string> TextProperty = AvaloniaProperty.Register<EditLabel, string>(nameof(Text));
        
        /// <summary>
        /// Close command property
        /// </summary>
        public static readonly StyledProperty<ICommand?> CloseCommandProperty = AvaloniaProperty.Register<EditLabel, ICommand?>(nameof(CloseCommand));
        
        /// <summary>
        /// Text binding
        /// </summary>
        public string Text
        {
            get { return GetValue(TextProperty); }
            set { SetValue(TextProperty, value); }
        }
        
        /// <summary>
        /// Command binding
        /// </summary>
        public ICommand? CloseCommand
        {
            get { return GetValue(CloseCommandProperty); }
            set { SetValue(CloseCommandProperty, value); }
        }
        
        public EditLabel()
        {
            InitializeComponent();

            // Bind focus events
            Label.Events().Tapped.Subscribe(_ => SetEditFocus(true));
            TextBox.Events().LostFocus.Subscribe(_ => SetEditFocus(false));

            // Bind close events
            HitArea.Events().PointerEntered.Subscribe(x => CloseButton.IsVisible = CloseCommand != null);
            HitArea.Events().PointerExited.Subscribe(x => CloseButton.IsVisible = false);
            
            // Escape focus on enter key
            TextBox.Events().KeyUp.Subscribe(e =>
            {
                if (e.Key == Avalonia.Input.Key.Enter)
                {
                    SetEditFocus(false);
                }
            });
        }

        /// <summary>
        /// Set the current edit focus
        /// </summary>
        /// <param name="isEdit"></param>
        private void SetEditFocus(bool isEdit)
        {
            Label.IsVisible = !isEdit;
            TextBox.IsVisible = isEdit;
        }
    }
}