namespace Studio.Models.Workspace
{
    public enum ConnectionStatus
    {
        /// <summary>
        /// No connection state
        /// </summary>
        None,
        
        /// <summary>
        /// Awaiting server response
        /// </summary>
        Connecting,
        
        /// <summary>
        /// Host resolver has rejected the GUID
        /// </summary>
        ResolveRejected,
        
        /// <summary>
        /// Host resolver has accepted the GUID, and is awaiting server response
        /// </summary>
        ResolveAccepted,
        
        /// <summary>
        /// Application has rejected the request
        /// </summary>
        ApplicationRejected,
        
        /// <summary>
        /// Application has accepted the request, bridge is now connected
        /// </summary>
        ApplicationAccepted
    }
}