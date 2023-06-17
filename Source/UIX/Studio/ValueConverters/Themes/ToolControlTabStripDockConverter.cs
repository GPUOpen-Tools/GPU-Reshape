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
using System.Globalization;
using Avalonia;
using Avalonia.Data.Converters;
using Avalonia.Layout;
using Dock.Model.Core;
using Dock.Model.ReactiveUI.Controls;

namespace Studio.ValueConverters.Themes
{
    public class ToolControlTabStripDockConverter : IValueConverter
    {
        /// <summary>
        /// Handle conversion to target
        /// </summary>
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            if (value is not ToolDock tool)
            {
                throw new NotSupportedException();
            }

            // Handle type
            if (targetType == typeof(Avalonia.Controls.Dock))
            {
                return tool.Alignment == Alignment.Right
                    ? Avalonia.Controls.Dock.Right
                    : Avalonia.Controls.Dock.Left;
            } 
            else if (targetType == typeof(Thickness))
            {
                return tool.Alignment == Alignment.Right
                    ? new Thickness(0, 0, 30, 0)
                    : new Thickness(30, 0, 0, 0);
            }
            else if (targetType == typeof(VerticalAlignment))
            {
                return tool.Alignment == Alignment.Bottom
                    ? VerticalAlignment.Bottom
                    : VerticalAlignment.Top;
            }
            else
            {
                throw new NotSupportedException();
            }
        }

        /// <summary>
        /// Backwards conversion not supported
        /// </summary>
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            throw new NotSupportedException();
        }
    }
}