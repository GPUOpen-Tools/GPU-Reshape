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

using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Avalonia.VisualTree;
using ReactiveUI;

namespace Studio.Views.Controls
{
    public partial class WindowChrome : UserControl
    {
        /// <summary>
        /// Title style for the given view
        /// </summary>
        public static readonly StyledProperty<string> TitleProperty = AvaloniaProperty.Register<WindowChrome, string>(nameof(Title));
        
        /// <summary>
        /// Title style for the given view
        /// </summary>
        public static readonly StyledProperty<bool> SeparatorProperty = AvaloniaProperty.Register<WindowChrome, bool>(nameof(Separator));
        
        /// <summary>
        /// Expansion command style for the given view
        /// </summary>
        public static readonly StyledProperty<ICommand> CloseCommandProperty = AvaloniaProperty.Register<SectionView, ICommand>(nameof(CloseCommand));
        
        /// <summary>
        /// Title getter / setter
        /// </summary>
        public string Title
        {
            get => GetValue(TitleProperty);
            set => SetValue(TitleProperty, value);
        }
        
        /// <summary>
        /// Separator getter / setter
        /// </summary>
        public bool Separator
        {
            get => GetValue(SeparatorProperty);
            set => SetValue(SeparatorProperty, value);
        }

        /// <summary>
        /// Close command getter / setter
        /// </summary>
        public ICommand CloseCommand
        {
            get => GetValue(CloseCommandProperty);
            set => SetValue(CloseCommandProperty, value);
        }

        public WindowChrome()
        {
            CloseCommand = ReactiveCommand.Create(OnClose);
        }

        /// <summary>
        /// Invoked on closes
        /// </summary>
        private void OnClose()
        {
            (this.GetVisualRoot() as Window)?.Close();
        }
    }
}