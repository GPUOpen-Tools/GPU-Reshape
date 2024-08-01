using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

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

    public static class PropertyViewModelWorkspaceExtensions
    {
        /// <summary>
        /// Get the workspace of a property
        /// </summary>
        public static IWorkspaceViewModel? GetWorkspace(this IPropertyViewModel self)
        {
            return self.GetFirstRoot<IWorkspaceAdapter>()?.GetWorkspace();
        }
        
        /// <summary>
        /// Get the workspace property collection of a property
        /// </summary>
        public static IPropertyViewModel? GetWorkspaceCollection(this IPropertyViewModel self)
        {
            return self.GetWorkspace()?.PropertyCollection;
        }
    }
}