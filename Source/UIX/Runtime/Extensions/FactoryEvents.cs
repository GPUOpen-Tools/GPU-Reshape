using System;
using System.Reactive.Linq;
using Dock.Model.Core;
using Dock.Model.Core.Events;

namespace Studio.Extensions
{
    public class FactoryEvents
    {
        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="factory">source factor</param>
        public FactoryEvents(IFactory factory)
        {
            this._factory = factory;
        }
        
        /// <summary>
        /// Observable closed event
        /// </summary>
        public IObservable<IDockable?> DockableClosed => Observable.FromEvent<EventHandler<DockableClosedEventArgs>, IDockable?>(handler =>
        {
            return (s, e) => handler(e.Dockable);
        }, handler => _factory.DockableClosed += handler, handler => _factory.DockableClosed -= handler);

        /// <summary>
        /// Internal factory
        /// </summary>
        private readonly IFactory _factory;
    }
}