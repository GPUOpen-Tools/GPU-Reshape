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
        }

        /// <summary>
        /// Internal service
        /// </summary>
        private HostResolver.CLR.HostResolverService? _service;
    }
}
