using System;
using System.Collections.Generic;
using System.Globalization;
using Avalonia.Data.Converters;
using Studio.Models.Diagnostic;

namespace Studio.ValueConverters
{
    public class InstrumentationStageConverter : IMultiValueConverter
    {
        /// <summary>
        /// Convert the value
        /// </summary>
        public object Convert(IList<object?> values, Type targetType, object? parameter, CultureInfo culture)
        {
            if (values.Count != 2u)
            {
                throw new Exception("Invalid value count");
            }
            
            // Translate count
            switch ((InstrumentationStage)(values[0] ?? throw new Exception("Invalid value")))
            {
                default:
                    return string.Empty;
                case InstrumentationStage.Shaders:
                    return string.Format(Resources.Resources.Instrumentation_Stage_Shaders, (int)(values[1] ?? throw new Exception("Invalid parameter")));
                case InstrumentationStage.Pipeline:
                    return string.Format(Resources.Resources.Instrumentation_Stage_Pipelines, (int)(values[1] ?? throw new Exception("Invalid parameter")));
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