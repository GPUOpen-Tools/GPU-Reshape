using Discovery.CLR;

namespace Studio.Services
{
    public class BackendDiscoveryService : IBackendDiscoveryService
    {
        /// <summary>
        /// Underlying CLR service
        /// </summary>
        public DiscoveryService? Service { get; }

        public BackendDiscoveryService()
        {
            // Create service
            Service = new DiscoveryService();
            
            // Attempt to install
            if (!Service.Install())
            {
                Service = null;
            }
        }
    }
}