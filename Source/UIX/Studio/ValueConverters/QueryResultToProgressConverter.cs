// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
using Avalonia;
using Avalonia.Data.Converters;
using Avalonia.Media;
using Runtime.Models.Query;
using Studio.Models.Workspace;
using Studio.ViewModels;

namespace Studio.ValueConverters
{
    public class QueryResultToProgressConverter : IValueConverter
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
        public object Convert(object? value, Type targetType, object? parameter, CultureInfo language)
        {
            if (value is not QueryResult status)
            {
                throw new NotSupportedException();
            }

            if (targetType == typeof(bool))
            {
                return ToEnabled(status);
            }
            else if(targetType == typeof(IBrush))
            {
                return ToColorBrush(status);
            }
            else if(targetType == typeof(string))
            {
                return ToMessage(status);
            }
            else
            {
                throw new NotSupportedException();
            }
        }

        /// <summary>
        /// Convert status to color brush
        /// </summary>
        /// <param name="status">connection status</param>
        /// <returns>color brush</returns>
        private SolidColorBrush ToColorBrush(QueryResult status)
        {
            var _default = new SolidColorBrush(Colors.Black);
            
            switch (status)
            {
                default:
                    return ResourceLocator.GetResource<SolidColorBrush>("DockApplicationAccentBrushLow") ?? _default;
                case QueryResult.DuplicateKey:
                case QueryResult.Invalid:
                    return ResourceLocator.GetResource<SolidColorBrush>("ErrorBrush") ?? _default;
                case QueryResult.OK:
                    return ResourceLocator.GetResource<SolidColorBrush>("DockApplicationAccentBrushHigh") ?? _default;
            }
        }

        /// <summary>
        /// Convert status to enabled state
        /// </summary>
        /// <param name="status">connection status</param>
        /// <returns>state</returns>
        private bool ToEnabled(QueryResult status)
        {
            switch (status)
            {
                default:
                    return false;
                case QueryResult.OK:
                    return true;
            }
        }

        /// <summary>
        /// Convert status to message
        /// </summary>
        /// <param name="status">connection status</param>
        /// <returns>message string</returns>
        public string ToMessage(QueryResult status)
        {
            switch (status)
            {
                default:
                    return "";
                case QueryResult.Invalid:
                    return "Failed to parse filter query";
                case QueryResult.DuplicateKey:
                    return "Filter query has a duplicate key";
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
        public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo language)
        {
            throw new NotSupportedException();
        }
    }
}