using System;
using System.Globalization;
using System.Linq;
using Avalonia.Data.Converters;
using Avalonia.Media;
using Studio.ViewModels.Tools;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ValueConverters
{
    public class PropertyToIconConverter : IValueConverter
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
            IPropertyViewModel property = value as IPropertyViewModel ?? throw new NotSupportedException("Expected property view model in conversion");

            // Handle target
            if (targetType == typeof(Geometry))
            {
                // Known cases
                switch (property)
                {
                    case WorkspaceCollectionViewModel:
                        return ResourceLocator.GetIcon("StreamOn");
                    case PipelineCollectionViewModel:
                    case ShaderCollectionViewModel:
                        return ResourceLocator.GetIcon("DotsGrid");
                    case IInstrumentationProperty:
                        return ResourceLocator.GetIcon("Circle");
                }

                // Special instrumentation
                if (property is IInstrumentableObject instrumentableObject)
                {
                    return ResourceLocator.GetIcon(instrumentableObject.GetOrCreateInstrumentationProperty().GetProperties<IInstrumentationProperty>().Any() ? "Record" : "RecordHollow");
                }
            }
            else if (targetType == typeof(Double))
            {
                // Known cases
                switch (property)
                {
                    default:
                        return 9;
                    case IInstrumentationProperty:
                        return 4;
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