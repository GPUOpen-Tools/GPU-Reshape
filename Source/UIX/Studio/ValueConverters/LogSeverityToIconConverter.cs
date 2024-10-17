// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
                        color =  ResourceLocator.GetResource<Color>("WarningDefaultColor");
                        break;
                    case LogSeverity.Error:
                        color = ResourceLocator.GetResource<Color>("ErrorDefaultColor");
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