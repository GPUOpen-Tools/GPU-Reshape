using DynamicData;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace
{
    public interface IWorkspaceViewModel
    {
        /// <summary>
        /// Base property collection
        /// </summary>
        public IPropertyViewModel PropertyCollection { get; }
        
        /// <summary>
        /// Active connection
        /// </summary>
        public IConnectionViewModel? Connection { get; }
    }
}