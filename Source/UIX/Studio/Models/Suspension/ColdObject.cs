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
using System.Collections.Generic;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace Studio.Models.Suspension
{
    public class ColdObject
    {
        /// <summary>
        /// All serialized data
        /// </summary>
        public Dictionary<string, ColdValue> Storage = new();

        /// <summary>
        /// Get or add a new storage object of type
        /// </summary>
        /// <param name="key">given key</param>
        /// <param name="type">expected type</param>
        /// <returns></returns>
        /// <exception cref="Exception"></exception>
        public T GetOrAdd<T>(string key, Type? type = null) where T : new()
        {
            if (Storage.TryGetValue(key, out ColdValue? coldValue))
            {
                // Needs unwrapping?
                if (coldValue.Value is JContainer container)
                {
                    try
                    {
                        coldValue.Value = container.ToObject<T>() ?? throw new Exception("Failed cold object instantiation");
                    }
                    catch (JsonSerializationException)
                    {
                        coldValue.Value = new T();
                    }
                }

                // Is of type?
                if (coldValue.Value is T value)
                {
                    return value;
                }
            }
            
            // Instantiate
            var obj = new T();
            
            // Create value
            Storage[key] = new ColdValue()
            {
                Type = type,
                Value = obj
            };
            
            // OK
            return obj;
        }

        /// <summary>
        /// Get a hot value
        /// </summary>
        /// <param name="key">expected key</param>
        /// <param name="type">type for dynamic instantiation</param>
        /// <returns></returns>
        public object? GetHotValue(string key, Type type)
        {
            ColdValue coldValue = Storage[key];
            
            // Needs unwrapping?
            if (coldValue.Value is JContainer container)
            {
                coldValue.Value = container.ToObject(type);
            }

            return coldValue.Value;
        }

        /// <summary>
        /// Try to get a hot value
        /// </summary>
        /// <param name="key">key to check for</param>
        /// <param name="type">type for dynamic instantiation</param>
        /// <param name="value">output value</param>
        /// <returns></returns>
        public bool TryGetHotValue(string key, Type type, out object? value)
        {
            if (!Storage.TryGetValue(key, out ColdValue? coldValue))
            {
                value = null;
                return false;
            }
            
            // Needs unwrapping?
            if (coldValue.Value is JContainer container)
            {
                coldValue.Value = container.ToObject(type);
            }

            value = coldValue.Value;
            return true;
        }
    }
}