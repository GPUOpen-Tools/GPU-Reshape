using Runtime.Models.Objects;

namespace Studio.ViewModels.Traits
{
    public interface IInstrumentationProperty
    {
        /// <summary>
        /// Commit all state
        /// </summary>
        /// <param name="state"></param>
        public void Commit(InstrumentationState state);
    }
}
