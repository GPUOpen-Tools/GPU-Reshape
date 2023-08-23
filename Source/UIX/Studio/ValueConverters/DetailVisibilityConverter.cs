using System;
using System.Globalization;
using Avalonia.Controls;
using Avalonia.Data.Converters;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ValueConverters
{
    public class DetailVisibilityConverter : IValueConverter
    {
        /// <summary>
        /// Convert the value
        /// </summary>
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            // Do not show on missing
            if (value is MissingDetailViewModel)
            {
                return false;
            }

            // Is detail?
            return value is IValidationDetailViewModel;
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