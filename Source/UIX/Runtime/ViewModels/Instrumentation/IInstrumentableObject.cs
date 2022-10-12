using Runtime.Models.Objects;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Instrumentation
{
    public interface IInstrumentableObject
    {
        /// <summary>
        /// Get the owning workspace
        /// </summary>
        /// <returns>property collection</returns>
        public IPropertyViewModel? GetWorkspace();
        
        /// <summary>
        /// Set the instrumentation state on this object
        /// </summary>
        /// <param name="state"></param>
        public void SetInstrumentation(InstrumentationState state);
    }
}