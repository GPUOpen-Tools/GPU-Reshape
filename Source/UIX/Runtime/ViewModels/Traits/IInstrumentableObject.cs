using Message.CLR;
using Runtime.Models.Objects;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Traits
{
    public interface IInstrumentableObject
    {
        /// <summary>
        /// Get the owning workspace
        /// </summary>
        /// <returns>property collection</returns>
        public IPropertyViewModel? GetWorkspace();

        /// <summary>
        /// Get the targetable instrumentation property
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel? GetOrCreateInstrumentationProperty();
        
        /// <summary>
        /// Current instrumentation state
        /// </summary>
        public InstrumentationState InstrumentationState { get; }
    }
}