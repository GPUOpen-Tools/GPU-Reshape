using System.Collections.Generic;
using System.Collections.ObjectModel;
using DynamicData;
using ReactiveUI;
using Studio.Models.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Objects
{
    public class ResourceValidationObject : ReactiveObject
    {
        public string DecoratedName
        {
            get => _decoratedName;
            set => this.RaiseAndSetIfChanged(ref _decoratedName, value);
        }

        /// <summary>
        /// Name of this object
        /// </summary>
        public Resource Resource
        {
            get => _resource;
            set
            {
                this.RaiseAndSetIfChanged(ref _resource, value);
                DecoratedName = $"{_resource.Name} : {_resource.Version}";
            }
        }

        /// <summary>
        /// All instances associated to this object
        /// </summary>
        public ObservableCollection<ResourceValidationInstance> Instances { get; } = new();

        /// <summary>
        /// Add a unique instance, only filtered against other invocations of AddUniqueInstance
        /// </summary>
        /// <param name="message"></param>
        public void AddUniqueInstance(string message)
        {
            // Existing?
            if (_unique.TryGetValue(message, out ResourceValidationInstance? instance))
            {
                instance.Count++;
                return;
            }

            // Create new instance
            instance = new ResourceValidationInstance();
            instance.Message = message;
            
            // Add to trackers
            Instances.Insert(0, instance);
            _unique.Add(message, instance);
            
            // Trim
            while (Instances.Count > MaxInstances)
            {
                _unique.Remove(Instances[^1].Message);
                Instances.RemoveAt(Instances.Count - 1);
            }
        }

        /// <summary>
        /// Max number of instances
        /// </summary>
        private static int MaxInstances = 1024;
        
        /// <summary>
        /// Unique lookup table
        /// </summary>
        private Dictionary<string, ResourceValidationInstance> _unique = new();

        /// <summary>
        /// Internal resource
        /// </summary>
        private Resource _resource;

        /// <summary>
        /// Internal name
        /// </summary>
        private string _decoratedName;
    }
}