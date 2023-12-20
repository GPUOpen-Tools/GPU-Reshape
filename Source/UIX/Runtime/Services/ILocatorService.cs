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

namespace Studio.Services
{
    public interface ILocatorService
    {
        /// <summary>
        /// Add a derived type
        /// </summary>
        /// <param name="type">source type</param>
        /// <param name="derived">derived type</param>
        public void AddDerived(Type type, Type derived);
        
        /// <summary>
        /// Get the derived type
        /// </summary>
        /// <param name="type">source type</param>
        /// <returns></returns>
        public Type? GetDerived(Type type);
    }

    public static class LocatorServiceExtensions
    {
        /// <summary>
        /// Instantiate a derived type
        /// </summary>
        /// <param name="service">self service</param>
        /// <param name="type">origin type to instantiate from</param>
        /// <typeparam name="T">casted type</typeparam>
        /// <returns>null if failed</returns>
        public static T? InstantiateDerived<T>(this ILocatorService service, Type type) where T : class
        {
            // Get derived
            Type? derived = service.GetDerived(type);
            if (derived == null)
            {
                return null;
            }
            
            // Instantiate as type
            return Activator.CreateInstance(derived) as T;
        }
        
        /// <summary>
        /// Instantiate a derived type
        /// </summary>
        /// <param name="service">self service</param>
        /// <param name="type">origin type to instantiate from</param>
        /// <param name="source">origin object to instantiate from</param>
        /// <returns>null if failed</returns>
        public static T? InstantiateDerived<T>(this ILocatorService service, object source) where T : class
        {
            return service.InstantiateDerived<T>(source.GetType());
        }
    }
}