using System.Reactive;
using System.Reactive.Subjects;
using ReactiveUI;

namespace Studio.Interactions
{
    public class DocumentInteractions
    {
        /// <summary>
        /// Shared open document handler
        /// </summary>
        public static Subject<object> OpenDocument = new();
    }
}