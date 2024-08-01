namespace Runtime.ViewModels.Traits
{
    public interface IPoolingConnection
    {
        /// <summary>
        /// Invoke an asynchronous discovery request
        /// </summary>
        public void DiscoverAsync();
    }
}