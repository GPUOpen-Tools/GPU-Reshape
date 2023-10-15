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

using System;
using System.Diagnostics;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Layout;

namespace Studio.Views.Controls
{
    public class ScrollingTabStripPanel : StackPanel
    {
        public ScrollingTabStripPanel()
        {
            // Always horizontal
            Orientation = Orientation.Horizontal;
        }

        /// <summary>
        /// Arrange override
        /// </summary>
        protected override Size ArrangeOverride(Size size)
        {
            // Create rect from offset
            Rect rect = new Rect(_offset, 0, size.Width, size.Height);
            
            // Last child size
            double previousChildSize = 0.0;
            
            // Handle all children
            foreach (IControl child in Children)
            {
                // Skip?
                if (child is not { IsVisible: true })
                {
                    continue;
                }

                // Direction?
                // Note: Avalonia StackPanel implementation
                if (Orientation == Orientation.Horizontal)
                {
                    rect = rect.WithX(rect.X + previousChildSize);
                    previousChildSize = child.DesiredSize.Width;
                    rect = rect.WithWidth(previousChildSize);
                    rect = rect.WithHeight(Math.Max(size.Height, child.DesiredSize.Height));
                    previousChildSize += Spacing;
                }
                else
                {
                    rect = rect.WithY(rect.Y + previousChildSize);
                    previousChildSize = child.DesiredSize.Height;
                    rect = rect.WithHeight(previousChildSize);
                    rect = rect.WithWidth(Math.Max(size.Width, child.DesiredSize.Width));
                    previousChildSize += Spacing;
                }

                // Arrange with size
                child.Arrange(rect);
            }
            
            return size;
        }

        /// <summary>
        /// Invoked on scrolling
        /// </summary>
        protected override void OnPointerWheelChanged(PointerWheelEventArgs e)
        {
            // TODO: Magic number
            _offset += e.Delta.X * 25.0;

            // Invalidate arrange if valid
            if (UpdateOffset())
            {
                InvalidateArrange();
            }
        }

        /// <summary>
        /// Update the scrolling
        /// </summary>
        /// <returns>true if changed</returns>
        private bool UpdateOffset()
        {
            // If the previous measurements aren't available yet, skip this
            if (double.IsNaN(DesiredSize.Width) || double.IsNaN(Bounds.Width))
            {
                return false;
            }

            // Measure from infinity
            Size unboundedMeasure = MeasureOverride(Size.Infinity);

            // Enough space?
            if (Bounds.Width > unboundedMeasure.Width)
            {
                bool invalidated = Math.Abs(_offset) > double.Epsilon;
                _offset = 0;
                return invalidated;
            }

            // Minimum offset
            double minOffset = Bounds.Width - unboundedMeasure.Width;

            // Clamp the value to safe bounds
            _offset = Math.Clamp(_offset, minOffset, 0);

            // Definitely changed
            return true;
        }

        /// <summary>
        /// Current offset
        /// </summary>
        private double _offset = 0;
    }
}