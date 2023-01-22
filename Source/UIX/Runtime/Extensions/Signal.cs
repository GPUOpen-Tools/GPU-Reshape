using System;
using System.Reactive;
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
    }
}