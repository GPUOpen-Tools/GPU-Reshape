﻿// 
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
using System.ComponentModel;
using System.Globalization;

namespace Studio.Views.Tools.Property
{
    public class AcceptAllConverter : TypeConverter
    {
        /// <summary>
        /// Target property info
        /// </summary>
        public Type TargetType;
        
        /// <summary>
        /// Gets a value indicating whether this converter can convert an object in the
        /// given source type to the native type of the converter.
        /// </summary>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            if (sourceType == TargetType)
            {
                return true;
            }

            return TypeDescriptor.GetConverter(TargetType).CanConvertFrom(sourceType);
        }

        /// <summary>
        /// Gets a value indicating whether the given value object is valid for this type.
        /// </summary>
        public override bool IsValid(ITypeDescriptorContext context, object? value)
        {
            if (value == null)
            {
                return false;
            }
            
            // If target type, just accept it
            if (value.GetType() == TargetType)
            {
                return true;
            }
            
            return TypeDescriptor.GetConverter(TargetType).IsValid(value);
        }

        /// <summary>
        /// Try to convert from a given object
        /// </summary>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object? value)
        {
            if (value == null)
            {
                return false;
            }
            
            // If target type, just accept it
            if (value.GetType() == TargetType)
            {
                return value;
            }

            // Catch any formatting issues
            try
            {
                return TypeDescriptor.GetConverter(TargetType).ConvertFrom(value)!;
            }
            catch
            {
                return null!;
            }
        }

        /// <summary>
        /// Gets a collection of standard values for the data type this type converter is designed for.
        /// </summary>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context)
        {
            return new StandardValuesCollection(new object[] { });
        }

        /// <summary>
        /// Gets a value indicating whether the collection of standard values returned from
        /// <see cref='System.ComponentModel.TypeConverter.GetStandardValues()'/> is an exclusive list.
        /// </summary>
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) => true;

        /// <summary>
        /// Gets a value indicating whether this object supports a standard set of values
        /// that can be picked from a list.
        /// </summary>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) => true;
    }
}