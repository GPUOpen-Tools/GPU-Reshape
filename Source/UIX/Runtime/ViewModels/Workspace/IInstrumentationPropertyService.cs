using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace
{
    public interface IInstrumentationPropertyService : IPropertyService
    {
        /// <summary>
        /// Name of this feature
        /// </summary>
        public string Name { get; }
        
        /// <summary>
        /// Create an instrumentation property for a given target
        /// </summary>
        /// <param name="target"></param>
        /// <returns></returns>
        public IPropertyViewModel? CreateInstrumentationObjectProperty(IPropertyViewModel target);
    }
}
