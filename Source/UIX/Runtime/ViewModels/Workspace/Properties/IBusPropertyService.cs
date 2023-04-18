using DynamicData;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;

namespace Runtime.ViewModels.Workspace.Properties
{
    public interface IBusPropertyService : IPropertyService
    {
        /// <summary>
        /// Current bus mode
        /// </summary>
        public BusMode Mode { get; set; }
        
        /// <summary>
        /// All outstanding requests
        /// </summary>
        public ISourceList<IBusObject> Objects { get; }

        /// <summary>
        /// Enqueue a bus object, enqueuing guaranteed to be unique
        /// </summary>
        /// <param name="busObject"></param>
        public void Enqueue(IBusObject busObject);

        /// <summary>
        /// Commit all enqueued bus objects
        /// </summary>
        public void Commit();
    }
}