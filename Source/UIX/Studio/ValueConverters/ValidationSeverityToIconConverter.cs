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
using Avalonia.Data.Converters;
using Avalonia.Media;
using Studio.Models.Workspace.Objects;

namespace Studio.ValueConverters
{
    public class ValidationSeverityToIconConverter : IValueConverter
    {
        /// <summary>
        /// Convert source data
        /// </summary>
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            if (targetType == typeof(Geometry))
            {
                switch (value ?? ValidationSeverity.Info)
                {
                    case ValidationSeverity.Info:
                        return ResourceLocator.GetIcon("Alert");
                    case ValidationSeverity.Warning:
                        return ResourceLocator.GetIcon("Warning");
                    case ValidationSeverity.Error:
                        return ResourceLocator.GetIcon("AlertPentagon");
                    default:
                        return null;
                }
            }

            if (targetType == typeof(IBrush))
            {
                Color color;
                switch (value ?? ValidationSeverity.Info)
                {
                    case ValidationSeverity.Info:
                        color = ResourceLocator.GetResource<Color>("SystemBaseHighColor");
                        break;
                    case ValidationSeverity.Warning:
                        color =  ResourceLocator.GetResource<Color>("WarningDefaultColor");
                        break;
                    case ValidationSeverity.Error:
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
        public object? ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}