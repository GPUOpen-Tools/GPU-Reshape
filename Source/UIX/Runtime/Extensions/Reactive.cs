using System;
using System.Reactive;
using System.Reactive.Linq;
using System.Windows.Input;
using ReactiveUI;

namespace Studio.Extensions
{
    public static class Reactive
    {
        /// <summary>
        /// Create a future command
        /// </summary>
        /// <param name="functor">future command fetcher</param>
        /// <returns></returns>
        public static ICommand Future(Func<ICommand?> functor)
        {
            return ReactiveCommand.Create<object?>(x => functor()?.Execute(x));
        }
    }
}
