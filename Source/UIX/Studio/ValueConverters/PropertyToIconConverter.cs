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