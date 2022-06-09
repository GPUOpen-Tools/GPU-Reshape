namespace Studio.Models
{
    public class HostResolverService
    {
        public HostResolverService()
        {
            _service = new HostResolver.CLR.HostResolverService();
        }

        // Start the underlying service
        public bool Start()
        {
            return _service.Install();
        }

        private readonly HostResolver.CLR.HostResolverService _service;
    }
}