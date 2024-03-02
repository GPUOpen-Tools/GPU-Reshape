using Studio.ViewModels.Workspace;

namespace Runtime.ViewModels.Traits
{
    public interface IWorkspaceAdapter
    {
        /// <summary>
        /// Get the workspace represented by this adapter
        /// </summary>
        /// <returns></returns>
        public IWorkspaceViewModel GetWorkspace();
    }
}