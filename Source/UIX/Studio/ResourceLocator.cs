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

using Avalonia;
using Avalonia.Media;

namespace Studio
{
    public class ResourceLocator
    {
        /// <summary>
        /// Get a default style resource
        /// </summary>
        /// <param name="name">key of the resource</param>
        /// <param name="defaultValue">default value if not found</param>
        /// <typeparam name="T">expected type</typeparam>
        /// <returns>found value or default</returns>
        public static T? GetResource<T>(string name, T? defaultValue = default)
        {
            object? value = null;
            
            if (!App.DefaultStyle.TryGetResource(name, out value) || value == null)
            {
                return defaultValue;
            }

            return value is T ? (T)value : defaultValue;
        }

        /// <summary>
        /// Get an icon from key
        /// </summary>
        /// <param name="name">icon key</param>
        /// <returns>null if not found</returns>
        public static StreamGeometry? GetIcon(string name)
        {
            // May not exist
            if (!Application.Current!.Styles.TryGetResource(name, out object? resource))
            {
                return null;
            }

            return resource as StreamGeometry;
        }
    }
}