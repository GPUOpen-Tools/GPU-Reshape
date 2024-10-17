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
using System.Collections.Generic;
using System.ComponentModel;
using DynamicData;
using Studio.ViewModels.Data;
using Studio.ViewModels.Setting;

namespace Studio.Services
{
    public class DefaultServiceRegistry : IServiceRegistry
    {
        /// <summary>
        /// Add a new service
        /// </summary>
        /// <param name="type">associated interface type</param>
        /// <param name="service">service object</param>
        public void Add(Type type, object service)
        {
            _services.Add(type, service);
        }

        /// <summary>
        /// Get a service object
        /// </summary>
        /// <param name="type">interface type to query</param>
        /// <returns>null if not found</returns>
        public object? Get(Type type)
        {
            return _services.GetValueOrDefault(type);
        }
        
        /// <summary>
        /// All installed services
        /// </summary>
        private Dictionary<Type, object> _services = new ();
    }
}