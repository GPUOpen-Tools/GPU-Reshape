using Avalonia;
using Avalonia.Media;

namespace Studio
{
    public class ResourceLocator
    {
        /// <summary>
        /// Get a default style resource
        /// </summary>
        /// <param name="name">key of the resource</param>
        /// <param name="defaultValue">default value if not found</param>
        /// <typeparam name="T">expected type</typeparam>
        /// <returns>found value or default</returns>
        public static T? GetResource<T>(string name, T? defaultValue = default)
        {
            object? value = null;
            
            if (!App.DefaultStyle.TryGetResource(name, out value) || value == null)
            {
                return defaultValue;
            }

            return value is T ? (T)value : defaultValue;
        }

        /// <summary>
        /// Get an icon from key
        /// </summary>
        /// <param name="name">icon key</param>
        /// <returns>null if not found</returns>
        public static StreamGeometry? GetIcon(string name)
        {
            // May not exist
            if (!Application.Current!.Styles.TryGetResource(name, out object? resource))
            {
                return null;
            }

            return resource as StreamGeometry;
        }
    }
}