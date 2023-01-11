using System;

namespace Studio.Services
{
    public interface ILocatorService
    {
        /// <summary>
        /// Add a derived type
        /// </summary>
        /// <param name="type">source type</param>
        /// <param name="derived">derived type</param>
        public void AddDerived(Type type, Type derived);
        
        /// <summary>
        /// Get the derived type
        /// </summary>
        /// <param name="type">source type</param>
        /// <returns></returns>
        public Type? GetDerived(Type type);
    }

    public static class LocatorServiceExtensions
    {
        /// <summary>
        /// Instantiate a derived type
        /// </summary>
        /// <param name="service">self service</param>
        /// <param name="type">origin type to instantiate from</param>
        /// <typeparam name="T">casted type</typeparam>
        /// <returns>null if failed</returns>
        public static T? InstantiateDerived<T>(this ILocatorService service, Type type) where T : class
        {
            // Get derived
            Type? derived = service.GetDerived(type);
            if (derived == null)
            {
                return null;
            }
            
            // Instantiate as type
            return Activator.CreateInstance(derived) as T;
        }
        
        /// <summary>
        /// Instantiate a derived type
        /// </summary>
        /// <param name="service">self service</param>
        /// <param name="type">origin type to instantiate from</param>
        /// <param name="source">origin object to instantiate from</param>
        /// <returns>null if failed</returns>
        public static T? InstantiateDerived<T>(this ILocatorService service, object source) where T : class
        {
            return service.InstantiateDerived<T>(source.GetType());
        }
    }
}