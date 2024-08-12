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

using System.Collections.Generic;

namespace Studio.ViewModels.Controls
{
    public class ObjectDictionary<KEY, VALUE> 
        where KEY : class 
        where VALUE : class
    {
        /// <summary>
        /// Add a new bidirectional pair
        /// </summary>
        public void Add(KEY key, VALUE value)
        {
            Forward.Add(key, value);
            Backward.Add(value, key);
        }

        /// <summary>
        /// Remove a bidirectional pair
        /// </summary>
        public void Remove(KEY key, VALUE value)
        {
            Forward.Remove(key);
            Backward.Remove(value);
        }

        /// <summary>
        /// Get the forward value
        /// </summary>
        public VALUE Get(KEY key)
        {
            return Forward[key];
        }

        /// <summary>
        /// Get the backward value
        /// </summary>
        public KEY Get(VALUE key)
        {
            return Backward[key];
        }
        
        /// <summary>
        /// All forward items
        /// </summary>
        public Dictionary<KEY, VALUE> Forward { get; } = new();
        
        /// <summary>
        /// All backward items
        /// </summary>
        public Dictionary<VALUE, KEY> Backward { get; } = new();
    }
}
