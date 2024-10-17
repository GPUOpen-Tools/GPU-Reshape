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
using System.Runtime.InteropServices;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Platform;
using ReactiveUI;

namespace Studio.Views
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
#if DEBUG
            this.AttachDevTools();
#endif
            
            // Bind state change
            this.WhenAnyValue(x => x.WindowState).Subscribe(OnWindowStateChanged);
        }

        /// <summary>
        /// Invoked on state changes
        /// </summary>
        private void OnWindowStateChanged(WindowState newState)
        {
            // Chrome-less windows may extend beyond the screen's working area in Avalonia 11.
            // This has been fixed in nightly, however, latest seems to have an issue regarding
            // visual lines and background colors. So, let's do a workaround for now.
            if (newState != WindowState.Maximized)
            {
                return;
            }

            // Get screen and handle
            if (Screens.ScreenFromWindow(this) is not { } screen || TryGetPlatformHandle() is not { } handle)
            {
                return;
            }

            // Is window outside of screen?
            if (screen.WorkingArea.Height >= ClientSize.Height * screen.Scaling || (Position.X > 0 && Position.Y > 0))
            {
                return;
            }

            // Adjust window position to fit the working area
            ClientSize = screen.WorkingArea.Size.ToSize(screen.Scaling);
            Position = screen.WorkingArea.Position;

            // Manually set the position and area
            Platform.Win32.SetWindowPos(
                handle.Handle, IntPtr.Zero,
                screen.WorkingArea.X, screen.WorkingArea.Y,
                screen.WorkingArea.Width, screen.WorkingArea.Height,
                64
            );
        }

        private void InitializeComponent()
        {
            AvaloniaXamlLoader.Load(this);
        }
    }
}
