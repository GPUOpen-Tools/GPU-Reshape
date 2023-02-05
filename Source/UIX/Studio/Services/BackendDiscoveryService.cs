using Avalonia;
using Discovery.CLR;
using DynamicData;
using Studio.Models.Logging;

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
                return;
            }

            // Detect conflicting instances
            if (Service.HasConflictingInstances())
            {
                App.Locator.GetService<ILoggingService>()?.ViewModel.Events.Add(new LogEvent()
                {
                    Severity = LogSeverity.Error,
                    Message = "Detected conflicting discovery services / instances, typically indicant of mixed GPU-Reshape installations, see Settings"
                });
            }
        }
    }
}