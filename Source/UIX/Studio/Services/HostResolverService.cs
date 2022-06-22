using Avalonia;
using Studio.Models.Logging;

namespace Studio.Services
{
    public class HostResolverService : IHostResolverService
    {
        /// <summary>
        /// Success state
        /// </summary>
        public bool Success { get; }

        public HostResolverService()
        {
            _service = new HostResolver.CLR.HostResolverService();
            
            // Install immediately
            Success = _service.Install();
            
            // Log it
            if (Success)
            {
                Logging.Info("Started host resolver service");
            }
            else
            {
                Logging.Error("Failed to start host resolver service");
            }
        }

        /// <summary>
        /// Internal service
        /// </summary>
        private HostResolver.CLR.HostResolverService? _service;
    }
}
