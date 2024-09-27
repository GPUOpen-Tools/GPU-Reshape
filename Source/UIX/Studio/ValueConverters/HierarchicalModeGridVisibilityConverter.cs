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
using Avalonia.Controls;
using Avalonia.Data.Converters;
using Studio.ViewModels.Workspace.Message;

namespace Studio.ValueConverters
{
    public class HierarchicalModeGridVisibilityConverter : IValueConverter
    {
        /// <summary>
        /// Shared length for icons
        /// </summary>
        public static GridLength Icon => new GridLength(25);
        
        /// <summary>
        /// Shared length for shader decorations
        /// </summary>
        public static GridLength Shader => new GridLength(200);
        
        /// <summary>
        /// Convert the value
        /// </summary>
        public object? Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        {
            HierarchicalMode mode   = (HierarchicalMode)(value ?? HierarchicalMode.None);
            HierarchicalMode target = (HierarchicalMode)(parameter ?? HierarchicalMode.None);

            // Test without negation
            bool test = mode.HasFlag(target & ~HierarchicalMode.Negate);

            // Apply negation
            if (target.HasFlag(HierarchicalMode.Negate))
            {
                test = !test;
            }
            
            // If passed, use the fallback value with <null>
            return test ? null : new GridLength(0.0);
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