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
using System.Collections.ObjectModel;

namespace Studio.Extensions
{
    public static class LinqExtensions
    {
        /// <summary>
        /// Functional iteration of a source
        /// </summary>
        public static void ForEach<T>(this IEnumerable<T> source, Action<T> action)
        {
            foreach (T item in source)
            {
                action(item);
            }
        }

        /// <summary>
        /// Promote a series of objects
        /// </summary>
        public static T[] Promote<T>(this IEnumerable<object> opaque) where T : class
        {
            List<T> promoted = new();

            foreach (object obj in opaque)
            {
                if (obj is T instance)
                {
                    promoted.Add(instance);
                }
            }

            return promoted.ToArray();
        }

        /// <summary>
        /// Promote a series of objects, if none are provided returns null
        /// </summary>
        public static T[]? PromoteOrNull<T>(this IEnumerable<object> opaque) where T : class
        {
            List<T> promoted = new();

            foreach (object obj in opaque)
            {
                if (obj is T instance)
                {
                    promoted.Add(instance);
                }
            }

            if (promoted.Count == 0)
            {
                return null;
            }

            return promoted.ToArray();
        }

        /// <summary>
        /// Promote a single object only, if multiple or none are provided, returns null
        /// </summary>
        public static T? PromoteSingle<T>(this IEnumerable<object> opaque) where T : class
        {
            if (opaque is IList<object> list)
            {
                // Multiple objects not allowed
                if (list.Count != 1)
                {
                    return null;
                }
                
                return list[0] as T;
            }
            else
            {
                using IEnumerator<object> enumerator = opaque.GetEnumerator();

                // May be empty
                if (!enumerator.MoveNext())
                {
                    return null;
                }

                // Try to cast
                T? value = enumerator.Current as T;

                // Multiple objects not allowed
                if (enumerator.MoveNext())
                {
                    return null;
                }

                return value;
            }
        }

        /// <summary>
        /// Represent a single value as an enumerable
        /// </summary>
        public static IEnumerable<T>? SingleEnumerable<T>(this T? item)
        {
            if (item != null)
            {
                yield return item;
            }
        }

        /// <summary>
        /// Convert an enumerable to an observable collection
        /// </summary>
        public static ObservableCollection<T> ToObservableCollection<T>(this IEnumerable<T> source)
        {
            return new ObservableCollection<T>(source);
        }
    }
}