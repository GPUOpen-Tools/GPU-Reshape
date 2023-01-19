using System;
using System.Linq.Expressions;
using System.Reactive;
using System.Reactive.Disposables;
using System.Reactive.Linq;
using System.Reactive.Subjects;
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
