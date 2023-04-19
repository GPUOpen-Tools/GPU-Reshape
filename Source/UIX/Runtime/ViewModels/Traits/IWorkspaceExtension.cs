using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Traits
{
    public interface IWorkspaceExtension
    {
        /// <summary>
        /// Install this extension
        /// </summary>
        /// <param name="workspaceViewModel"></param>
        public void Install(IWorkspaceViewModel workspaceViewModel);
    }
}