using System;
using System.Linq.Expressions;
using System.Reactive;
using System.Reactive.Linq;
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
    }
}
