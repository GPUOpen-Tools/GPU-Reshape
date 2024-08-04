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

using System.Windows.Input;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Markup.Xaml;
using Avalonia.Media;
using Avalonia.VisualTree;
using ReactiveUI;
using Studio.Extensions;
using System;
using Avalonia.Controls.Primitives;

namespace Studio.Views.Controls
{
    public partial class WindowChrome : UserControl
    {
        /// <summary>
        /// Title style for the given view
        /// </summary>
        public static readonly StyledProperty<string> TitleProperty = AvaloniaProperty.Register<WindowChrome, string>(nameof(Title));
        
        /// <summary>
        /// Separator style for the given view
        /// </summary>
        public static readonly StyledProperty<bool> SeparatorProperty = AvaloniaProperty.Register<WindowChrome, bool>(nameof(Separator));
        
        /// <summary>
        /// Show icon style for the given view
        /// </summary>
        public static readonly StyledProperty<bool> ShowIconProperty = AvaloniaProperty.Register<WindowChrome, bool>(nameof(ShowIcon), true);
        
        /// <summary>
        /// Maximize icon style for the given view
        /// </summary>
        public static readonly StyledProperty<StreamGeometry> MaximizeIconProperty = AvaloniaProperty.Register<WindowChrome, StreamGeometry>(nameof(MaximizeIcon));
        
        /// <summary>
        /// Show min style for the given view
        /// </summary>
        public static readonly StyledProperty<bool> CanMinimizeProperty = AvaloniaProperty.Register<WindowChrome, bool>(nameof(CanMinimize));
        
        /// <summary>
        /// Show max style for the given view
        /// </summary>
        public static readonly StyledProperty<bool> CanMaximizeProperty = AvaloniaProperty.Register<WindowChrome, bool>(nameof(CanMaximize));
        
        /// <summary>
        /// Minimize command style for the given view
        /// </summary>
        public static readonly StyledProperty<ICommand> MinimizeCommandProperty = AvaloniaProperty.Register<SectionView, ICommand>(nameof(MinimizeCommand));
        
        /// <summary>
        /// Maximize command style for the given view
        /// </summary>
        public static readonly StyledProperty<ICommand> MaximizeCommandProperty = AvaloniaProperty.Register<SectionView, ICommand>(nameof(MaximizeCommand));
        
        /// <summary>
        /// Close command style for the given view
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
        /// Show icon getter / setter
        /// </summary>
        public bool ShowIcon
        {
            get => GetValue(ShowIconProperty);
            set => SetValue(ShowIconProperty, value);
        }
        
        /// <summary>
        /// Can minimize getter / setter
        /// </summary>
        public bool CanMinimize
        {
            get => GetValue(CanMinimizeProperty);
            set => SetValue(CanMinimizeProperty, value);
        }
        
        /// <summary>
        /// Can maximize getter / setter
        /// </summary>
        public bool CanMaximize
        {
            get => GetValue(CanMaximizeProperty);
            set => SetValue(CanMaximizeProperty, value);
        }
        
        /// <summary>
        /// Maximize icon getter / setter
        /// </summary>
        public StreamGeometry MaximizeIcon
        {
            get => GetValue(MaximizeIconProperty);
            set => SetValue(MaximizeIconProperty, value);
        }

        /// <summary>
        /// Minimize command getter / setter
        /// </summary>
        public ICommand MinimizeCommand
        {
            get => GetValue(MinimizeCommandProperty);
            set => SetValue(MinimizeCommandProperty, value);
        }

        /// <summary>
        /// Maximize command getter / setter
        /// </summary>
        public ICommand MaximizeCommand
        {
            get => GetValue(MaximizeCommandProperty);
            set => SetValue(MaximizeCommandProperty, value);
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
            MinimizeCommand = ReactiveCommand.Create(OnMinimize);
            MaximizeCommand = ReactiveCommand.Create(OnMaximize);
            CloseCommand = ReactiveCommand.Create(OnClose);
            UpdateIcons();
        }

        /// <summary>
        /// Invoked on template apply
        /// </summary>
        protected override void OnApplyTemplate(TemplateAppliedEventArgs e)
        {
            base.OnApplyTemplate(e);

            // Bind window dragging
            e.NameScope.Find<Grid>("PART_DragGrid")!
                .Events().PointerPressed
                .Subscribe(events =>
                {
                    if (events.GetCurrentPoint(this).Properties.IsLeftButtonPressed)
                    {
                        // TODO: This only holds true for Windows
                        Window?.BeginMoveDrag(events);
                    }
                });

            // Bind state changes
            if (Window is { } window)
            {
                window.GetPropertyChangedObservable(Window.WindowStateProperty).AddClassHandler<Visual>((t, args) =>
                {
                    UpdateIcons();
                });
            }
        }

        /// <summary>
        /// Update all icon states
        /// </summary>
        private void UpdateIcons()
        {
            if (Window?.WindowState == WindowState.Maximized)
            {
                MaximizeIcon = ResourceLocator.GetIcon("Restore")!;
            }
            else
            {
                MaximizeIcon = ResourceLocator.GetIcon("Maximize")!;
            }
        }

        /// <summary>
        /// Invoked on minimizes
        /// </summary>
        private void OnMinimize()
        {
            if (Window is { } window)
            {
                window.WindowState = WindowState.Minimized;
            }
        }

        /// <summary>
        /// Invoked on maximizes
        /// </summary>
        private void OnMaximize()
        {
            if (Window is not { } window)
            {
                return;
            }
            
            // Restore if already maximized
            if (window.WindowState == WindowState.Maximized)
            {
                window.WindowState = WindowState.Normal;
            }
            else
            {
                window.WindowState = WindowState.Maximized;
            }
        }

        /// <summary>
        /// Invoked on closes
        /// </summary>
        private void OnClose()
        {
            Window?.Close();
        }

        /// <summary>
        /// Window helper
        /// </summary>
        private Window? Window => this.GetVisualRoot() as Window;
    }
}