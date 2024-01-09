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
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Studio.Extensions
{
    public static class Formatting
    {
        /// <summary>
        /// Perform a natural language join, f.x. "A, B, C and D"
        /// </summary>
        /// <param name="source">items to join</param>
        public static string NaturalJoin<T>(this IEnumerable<T> source) where T : class
        {
            return NaturalJoin(source, x => x);
        }

        /// <summary>
        /// Perform a natural language join, f.x. "A, B, C and D"
        /// </summary>
        /// <param name="source">items to join</param>
        /// <param name="selector">selection criteria into source</param>
        public static string NaturalJoin<T>(this IEnumerable<T> source, Func<T, object> selector) where T : class
        {
            StringBuilder builder = new();

            // Flatten the array
            T[] array = source.ToArray();

            // Endpoints
            T first = array.First();
            T last = array.Last();
            
            // Compose message
            foreach (T item in array)
            {
                if (item != first)
                {
                    if (item == last)
                    {
                        builder.Append(" and ");
                    }
                    else
                    {
                        builder.Append(", ");
                    }
                }

                builder.Append(selector(item));
            }
            
            return builder.ToString();
        }
    }
}