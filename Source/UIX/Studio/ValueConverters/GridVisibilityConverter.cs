using System;
using System.Globalization;
using Avalonia.Controls;
using Avalonia.Data.Converters;

namespace Studio.ValueConverters
{
    public class GridVisibilityConverter : IValueConverter
    {
        /// <summary>
        /// Convert the value
        /// </summary>
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            // Show the definition?
            if (value is bool boolValue && boolValue)
            {
                // Assume length from parameter
                return new GridLength(double.Parse((string)parameter));
            }
            else
            {
                // Hidden
                return new GridLength(0.0);
            }
        }

        /// <summary>
        /// Convert the value back
        /// </summary>
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}