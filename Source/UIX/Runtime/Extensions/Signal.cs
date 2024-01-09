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
using System.Reactive;
using System.Reactive.Disposables;
using System.Reactive.Linq;
using System.Reactive.Subjects;
using DynamicData;
using ReactiveUI;

namespace Studio.Extensions
{
    public static class SignalExtensions
    {
        /// <summary>
        /// Convert an observable event to a parameter-less "signal"
        /// </summary>
        /// <param name="source">source observable</param>
        /// <typeparam name="T">ignored parameter type</typeparam>
        /// <returns>parameterless observable</returns>
        public static IObservable<Unit> ToSignal<T>(this IObservable<T> source)
        {
            return source.Select(_ => Unit.Default);
        }

        /// <summary>
        /// Cast a nullable observable
        /// </summary>
        /// <param name="source">source observable</param>
        /// <typeparam name="T">target type</typeparam>
        /// <returns>always valid</returns>
        public static IObservable<T> CastNullable<T>(this IObservable<object?> source) where T : class
        {
            return source.WhereNotNull().Cast<T>().WhereNotNull();
        }
        
        /// <summary>
        /// Dispose with a composite disposable with a pre-clear
        /// </summary>
        /// <param name="item">item to dispose with</param>
        /// <param name="compositeDisposable">container to dispose to</param>
        /// <returns>item</returns>
        /// <exception cref="ArgumentNullException"></exception>
        public static T DisposeWithClear<T>(this T item, CompositeDisposable compositeDisposable) where T : IDisposable
        {
            if (compositeDisposable is null)
            {
                throw new ArgumentNullException(nameof(compositeDisposable));
            }

            compositeDisposable.Clear();
            compositeDisposable.Add(item);
            return item;
        }
    }
}