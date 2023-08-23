// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights 
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished to do so, 
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all 
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 

using System.Collections.Generic;
using System.Collections.ObjectModel;
using ReactiveUI;
using Studio.Models.Workspace.Objects;

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
        /// Is the collection paused?
        /// </summary>
        public bool Paused
        {
            get => _paused;
            set
            {
                this.RaiseAndSetIfChanged(ref _paused, value);
                PauseChanged();
            }
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
        /// Invoked on pause state changes
        /// </summary>
        private void PauseChanged()
        {
            // Update all objects
            foreach (ResourceValidationObject validationObject in Resources)
            {
                validationObject.Paused = _paused;
            }
        }

        /// <summary>
        /// Find or add a resource
        /// </summary>
        /// <param name="resource"></param>
        /// <returns></returns>
        public ResourceValidationObject FindOrAddResource(Resource resource)
        {
            ulong key = resource.Key;
            
            // Existing?
            if (_lookup.TryGetValue(key, out ResourceValidationObject? validationObject))
            {
                // Recommit the resource info if needed
                if (validationObject.Resource.IsUnknown && !resource.IsUnknown)
                {
                    validationObject.Resource = resource;
                }
                
                return validationObject;
            }

            // Not found, create it
            validationObject = new ResourceValidationObject()
            {
                Resource = resource,
                Paused = _paused
            };

            // Auto-assign if none
            if (Resources.Count == 0)
            {
                SelectedResource = validationObject;
            }
            
            // Add to lookups
            _lookup.Add(key, validationObject);
            Resources.Insert(0, validationObject);

            // Trim
            while (Resources.Count > MaxResources)
            {
                _lookup.Remove(Resources[^1].Resource.Key);
                Resources.RemoveAt(Resources.Count - 1);
            }
            
            // OK
            return validationObject;
        }

        /// <summary>
        /// Max number of resources
        /// </summary>
        private static int MaxResources = 100;

        /// <summary>
        /// Lookup table
        /// </summary>
        private Dictionary<ulong, ResourceValidationObject> _lookup = new();

        /// <summary>
        /// Internal selection
        /// </summary>
        private ResourceValidationObject? _selectedResource;

        /// <summary>
        /// Internal pause state
        /// </summary>
        private bool _paused;
    }
}