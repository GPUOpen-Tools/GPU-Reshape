using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Shader
{
    public class NavigationLocation
    {
        /// <summary>
        /// Construct from a validation object
        /// </summary>
        public static NavigationLocation? FromObject(ValidationObject validationObject)
        {
            // If no segment has been bound, its invalid
            if (validationObject.Segment == null)
            {
                return null;
            }

            return new NavigationLocation()
            {
                Location = validationObject.Segment.Location,
                Object = validationObject
            };
        }
        
        /// <summary>
        /// Source location
        /// </summary>
        public ShaderLocation Location;

        /// <summary>
        /// Optional, the actual validation object
        /// </summary>
        public ValidationObject? Object;
    }
}