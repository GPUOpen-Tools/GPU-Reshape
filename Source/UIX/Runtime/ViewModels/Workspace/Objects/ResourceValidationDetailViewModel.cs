using System.Collections.Generic;
using System.Collections.ObjectModel;
using ReactiveUI;

namespace Studio.ViewModels.Workspace.Objects
{
    public class ResourceValidationDetailViewModel : ReactiveObject, IValidationDetailViewModel
    {
        /// <summary>
        /// Current selected resource
        /// </summary>
        public ResourceValidationObject? SelectedResource
        {
            get => _selectedResource;
            set => this.RaiseAndSetIfChanged(ref _selectedResource, value);
        }

        /// <summary>
        /// All resources
        /// </summary>
        public ObservableCollection<ResourceValidationObject> Resources { get; } = new();
        
        /// <summary>
        /// Find a resource
        /// </summary>
        /// <param name="id"></param>
        /// <returns></returns>
        public ResourceValidationObject? FindResource(uint id)
        {
            if (_lookup.TryGetValue(id, out ResourceValidationObject? validationObject))
            {
                return validationObject;
            }

            // Not found
            return null;
        }

        /// <summary>
        /// Find or add a resource
        /// </summary>
        /// <param name="id"></param>
        /// <returns></returns>
        public ResourceValidationObject FindOrAddResource(uint id)
        {
            // Existing?
            if (_lookup.TryGetValue(id, out ResourceValidationObject? validationObject))
            {
                return validationObject;
            }

            // Not found, create it
            validationObject = new ResourceValidationObject()
            {
                ID = id,
                DecoratedName = $"${id}"
            };

            // Auto-assign if none
            if (Resources.Count == 0)
            {
                SelectedResource = validationObject;
            }
            
            // Add to lookups
            _lookup.Add(id, validationObject);
            Resources.Add(validationObject);
            
            // OK
            return validationObject;
        }

        /// <summary>
        /// Lookup table
        /// </summary>
        private Dictionary<uint, ResourceValidationObject> _lookup = new();

        /// <summary>
        /// Internal selection
        /// </summary>
        private ResourceValidationObject? _selectedResource;
    }
}