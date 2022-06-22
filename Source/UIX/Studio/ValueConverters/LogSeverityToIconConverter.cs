using System;
using System.Globalization;
using Avalonia.Data.Converters;
using Studio.Models.Logging;
using Studio.ViewModels.Tools;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ValueConverters
{
    public class LogSeverityToIconConverter : IValueConverter
    {
        /// <summary>
        /// Convert source data
        /// </summary>
        /// <param name="value">inbound value</param>
        /// <param name="targetType">expected type</param>
        /// <param name="parameter">originating parameter</param>
        /// <param name="language">culture</param>
        /// <returns>converted value</returns>
        /// <exception cref="NotSupportedException"></exception>
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            switch (value ?? LogSeverity.Info)
            {
                case LogSeverity.Info:
                    return ResourceLocator.GetIcon("Alert");
                case LogSeverity.Warning:
                    return ResourceLocator.GetIcon("Warning");
                case LogSeverity.Error:
                    return ResourceLocator.GetIcon("AlertPentagon");
                default:
                    return null;
            }
        }

        /// <summary>
        /// Convert destination data
        /// </summary>
        /// <param name="value">inbound value</param>
        /// <param name="targetType">expected type</param>
        /// <param name="parameter">originating parameter</param>
        /// <param name="language">culture</param>
        /// <returns>converted value</returns>
        /// <exception cref="NotSupportedException"></exception>
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}