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

namespace Studio.Services
{
    public interface IServiceRegistry
    {
        /// <summary>
        /// Add a new service
        /// </summary>
        /// <param name="type">associated interface type</param>
        /// <param name="service">service object</param>
        public void Add(Type type, object service);

        /// <summary>
        /// Get a service object
        /// </summary>
        /// <param name="type">interface type to query</param>
        /// <returns>null if not found</returns>
        public object? Get(Type type);
    }

    public static class ServiceRegistry
    {
        /// <summary>
        /// Shared registry across the application
        /// </summary>
        public static IServiceRegistry Current = null!;
        
        /// <summary>
        /// Install the service registry
        /// </summary>
        /// <returns>registry</returns>
        public static T Install<T>() where T : IServiceRegistry, new()
        {
            T registry = new();
            Current = registry;
            return registry;
        }
        
        /// <summary>
        /// Add a service
        /// </summary>
        /// <param name="service">service object to add</param>
        /// <typeparam name="T">interface type</typeparam>
        public static void Add<T>(T service) where T : class
        {
            Current.Add(typeof(T), service);
        }
        
        /// <summary>
        /// Get a service
        /// </summary>
        /// <typeparam name="T">interface to query</typeparam>
        /// <returns>null if not found</returns>
        public static T? Get<T>() where T : class
        {
            return Current.Get(typeof(T)) as T;
        }
    }
}