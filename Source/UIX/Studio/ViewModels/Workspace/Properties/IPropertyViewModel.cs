using DynamicData;
using ReactiveUI;

namespace Studio.ViewModels.Workspace.Properties
{
    public interface IPropertyViewModel
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name { get; }
        
        /// <summary>
        /// Property collection within
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; }
    }
}