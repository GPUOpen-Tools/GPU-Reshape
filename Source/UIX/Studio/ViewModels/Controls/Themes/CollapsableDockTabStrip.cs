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
using Avalonia.Controls.Metadata;
using Avalonia.Controls.Primitives;
using Avalonia.Styling;
using Dock.Avalonia.Controls;

namespace Studio.ViewModels.Controls.Themes
{
    [PseudoClasses(":create")]
    public class CollapsableDockTabStrip : TabStrip, IStyleable
    {
        /// <summary>
        /// Item creation property
        /// </summary>
        public static readonly StyledProperty<bool> CanCreateItemProperty = AvaloniaProperty.Register<DockPanel, bool>(nameof(CanCreateItem));

        /// <summary>
        /// If an item can be created in this strip
        /// </summary>
        public bool CanCreateItem
        {
            get => GetValue(CanCreateItemProperty);
            set => SetValue(CanCreateItemProperty, value);
        }

        /// <summary>
        /// Styled as a TabStrip, not collapsable
        /// </summary>
        Type IStyleable.StyleKey => typeof(TabStrip);
        
        public CollapsableDockTabStrip()
        {
            // No multi-select
            SelectionMode = SelectionMode.Single;
            UpdatePseudoClasses(CanCreateItem);
        }

        /// <summary>
        /// Invoked on property changes
        /// </summary>
        /// <param name="change"></param>
        protected override void OnPropertyChanged(AvaloniaPropertyChangedEventArgs change)
        {
            base.OnPropertyChanged(change);

            if (change.Property == CanCreateItemProperty)
            {
                UpdatePseudoClasses(change.NewValue as bool? ?? false);
            }
        }

        /// <summary>
        /// Update all classes
        /// </summary>
        private void UpdatePseudoClasses(bool canCreate)
        {
            PseudoClasses.Set(":create", canCreate);
        }
    }
}