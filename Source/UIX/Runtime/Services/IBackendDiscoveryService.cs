using Discovery.CLR;

namespace Studio.Services
{
    public interface IBackendDiscoveryService
    {
        /// <summary>
        /// Underlying CLR service
        /// </summary>
        public DiscoveryService? Service { get; }
    }
}