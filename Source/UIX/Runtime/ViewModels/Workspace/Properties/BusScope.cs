using System;

namespace Runtime.ViewModels.Workspace.Properties
{
    public class BusScope : IDisposable
    {
        /// <summary>
        /// Create a bus scope
        /// </summary>
        /// <param name="service">target service</param>
        /// <param name="mode">temporary scope</param>
        public BusScope(IBusPropertyService? service, BusMode mode)
        {
            _service = service;

            // Null allowed
            if (service == null)
            {
                return;
            }
            
            // Cache and set scope
            _reconstruct = service.Mode;
            service.Mode = mode;
        }
        
        /// <summary>
        /// Dispose the scope
        /// </summary>
        public void Dispose()
        {
            // Reconstruct scope
            if (_service != null)
            {
                _service.Mode = _reconstruct;
            }
        }

        /// <summary>
        /// Internal service
        /// </summary>
        private IBusPropertyService? _service;
        
        /// <summary>
        /// Internal reconstruction state
        /// </summary>
        private BusMode _reconstruct;
    }
}