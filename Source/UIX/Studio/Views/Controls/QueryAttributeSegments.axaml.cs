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
using System.Collections.Generic;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Markup.Xaml;
using Avalonia.Media.Immutable;
using DynamicData;
using Runtime;
using Runtime.Models.Query;

namespace Studio.Views.Controls
{
    public partial class QueryAttributeSegments : UserControl
    {
        /// <summary>
        /// Source query box
        /// </summary>
        public AutoCompleteBox QueryBox;

        /// <summary>
        /// Source decorators
        /// </summary>
        public SourceList<QueryAttributeDecorator> Decorators
        {
            set
            {
                value.Connect()
                    .OnItemRemoved(OnDecoratorRemoved)
                    .OnItemAdded(OnDecoratorAdded)
                    .Subscribe();
            }
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public QueryAttributeSegments()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Invoked on decorator addition
        /// </summary>
        /// <param name="decorator"></param>
        private void OnDecoratorAdded(QueryAttributeDecorator decorator)
        {
            /**
             * NOTE: Workaround for odd internal ItemsControl behaviour with item removal,
             *       added manually instead of using generative models.
             */

            // Measure glyph begin
            double beginWidth = Math.Floor(TextFormatter.GetWidth(QueryBox.Text.Substring(0, decorator.Attribute.Offset), QueryBox.FontSize).Width);

            // Initial text block offset
            if (decorator.Attribute.Offset != 0)
            {
                beginWidth += 5;
            }

            // Measure glyph end
            double endWidth;
            if (decorator.Attribute.Offset + decorator.Attribute.Length <= QueryBox.Text.Length)
            {
                endWidth = Math.Floor(TextFormatter.GetWidth(QueryBox.Text.Substring(0, decorator.Attribute.Offset + decorator.Attribute.Length), QueryBox.FontSize).Width) + 5;
            }
            else
            {
                endWidth = beginWidth;
            }

            // Create decorator control
            var control = new Border()
            {
                Background = new ImmutableSolidColorBrush(decorator.Color),
                Width = endWidth - beginWidth,
                Margin = new Thickness(beginWidth, 0, 0, 0),
                Height = 1.25,
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top
            };

            // Add
            SegmentPanel.Children.Add(control);
            _segmentControls.Add(decorator, control);
        }

        /// <summary>
        /// Invoked on decorator removal
        /// </summary>
        /// <param name="decorator"></param>
        private void OnDecoratorRemoved(QueryAttributeDecorator decorator)
        {
            SegmentPanel.Children.Remove(_segmentControls[decorator]);
            _segmentControls.Remove(decorator);
        }

        /// <summary>
        /// Tracked decorators
        /// </summary>
        private Dictionary<QueryAttributeDecorator, Control> _segmentControls = new();
    }
}