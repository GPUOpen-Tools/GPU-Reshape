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
using System.Linq.Expressions;
using System.Reactive;
using System.Reactive.Disposables;
using System.Reactive.Linq;
using System.Reactive.Subjects;
using System.Runtime.CompilerServices;
using System.Windows.Input;
using ReactiveUI;

namespace Studio.Extensions
{
    public static class ReactiveObjectExtensions
    {
        /// <summary>
        /// Create a future command
        /// </summary>
        /// <param name="functor">future command fetcher</param>
        /// <returns></returns>
        public static void BindProperty<TSender, TObj>(this TSender source, Expression<Func<TSender, TObj>> getter, Action<TObj> setter)
        {
            source.WhenAnyValue(getter).Subscribe(setter);
        }
        
        /// <summary>
        /// Raise and set if changed
        /// </summary>
        /// <param name="source">source object</param>
        /// <param name="field">given field to check and assign</param>
        /// <param name="value">value to check against</param>
        /// <param name="propertyName">field name</param>
        /// <returns>returns true if changed</returns>
        public static bool CheckRaiseAndSetIfChanged<TObj, TRet>(this TObj source, ref TRet field, TRet value, [CallerMemberName] string? propertyName = null) where TObj : IReactiveObject
        {
            if (EqualityComparer<TRet>.Default.Equals(field, value))
            {
                return false;
            }

            // Proxy down
            source.RaiseAndSetIfChanged(ref field, value, propertyName);
            return true;
        }

        /// <summary>
        /// Create an observable for deactivation
        /// </summary>
        /// <param name="source">source object</param>
        /// <returns>observable</returns>
        public static IObservable<T> WhenDisposed<T>(this T source) where T: IActivatableViewModel
        {
            Subject<T> subject = new();
            
            // Invoke with disposable
            source.WhenActivated((CompositeDisposable disposable) =>
            {
                Disposable.Create(() => subject.OnNext(source)).DisposeWith(disposable);
            });

            return subject;
        }

        /// <summary>
        /// Create an observable for deactivation
        /// </summary>
        /// <param name="source">source object</param>
        /// <param name="actor">functor to be invoked on disposes</param>
        /// <returns>observable</returns>
        public static IDisposable WhenDisposed<T>(this T source, Action<T> actor) where T: IActivatableViewModel
        {
            return source.WhenDisposed().Subscribe(actor);
        }
    }
}
