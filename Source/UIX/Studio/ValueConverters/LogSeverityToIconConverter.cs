using System;
using System.Globalization;
using System.Resources;
using Avalonia.Data.Converters;
using Avalonia.Media;
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
            if (targetType == typeof(Geometry))
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

            if (targetType == typeof(IBrush))
            {
                Color color;
                switch (value ?? LogSeverity.Info)
                {
                    case LogSeverity.Info:
                        color = ResourceLocator.GetResource<Color>("SystemBaseHighColor");
                        break;
                    case LogSeverity.Warning:
                        color =  ResourceLocator.GetResource<Color>("WarningColor");
                        break;
                    case LogSeverity.Error:
                        color = ResourceLocator.GetResource<Color>("ErrorColor");
                        break;
                    default:
                        return null;
                }

                // Create brush
                return new SolidColorBrush(color);
            }

            return null;
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