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
                    ? new Thickness(0, 0, 34, 0)
                    : new Thickness(34, 0, 0, 0);
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