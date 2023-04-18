namespace Runtime.ViewModels.Workspace.Properties
{
    public enum BusMode
    {
        /// <summary>
        /// All bus requests are immediate
        /// </summary>
        Immediate,
        
        /// <summary>
        /// All bus requests are deferred to manual commits
        /// </summary>
        RecordAndCommit
    }
}
