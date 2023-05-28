using System;
using System.Globalization;
using System.Linq;
using Avalonia.Data.Converters;
using Avalonia.Media;
using Runtime.ViewModels.Shader;
using Studio.ViewModels.Tools;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ValueConverters
{
    public class FileToIconConverter : IValueConverter
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
            // Handle target
            if (targetType == typeof(Geometry))
            {
                if (value is ShaderFileViewModel)
                {
                    return ResourceLocator.GetIcon("CodeFileHollow");
                }
                else
                {
                    return ResourceLocator.GetIcon("FolderHollow");
                }
            }
            else if (targetType == typeof(Double))
            {
                if (value is ShaderFileViewModel)
                {
                    return 10.0;
                }
                else
                {
                    return 14.0;
                }
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