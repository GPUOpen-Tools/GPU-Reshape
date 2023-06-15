using System.Collections.Generic;
using System.Collections.ObjectModel;
using DynamicData;
using ReactiveUI;
using Runtime.Threading;
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

        public ResourceValidationObject()
        {
            // Bind pump
            pump.Bind(Instances);
        }

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
            pump.Insert(0, instance);
            _unique.Add(message, instance);
            
            // Trim
            while (pump.Count > MaxInstances)
            {
                _unique.Remove(pump[^1].Message);
                pump.RemoveAt(pump.Count - 1);
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
        /// Pump for main collection
        /// </summary>
        private PumpedList<ResourceValidationInstance> pump = new();

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