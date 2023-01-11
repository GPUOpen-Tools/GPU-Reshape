using Dock.Model.Core;

namespace Studio.Extensions
{
    public static class FactoryExtensioons
    {
        /// <summary>
        /// Create factory events
        /// </summary>
        /// <param name="factory">source factory</param>
        /// <returns></returns>
        public static FactoryEvents Events(this IFactory factory)
        {
            return new FactoryEvents(factory);
        }
    }
}