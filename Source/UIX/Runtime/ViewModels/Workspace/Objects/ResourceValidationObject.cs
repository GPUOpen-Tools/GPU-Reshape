using System.Collections.Generic;
using System.Collections.ObjectModel;
using DynamicData;
using ReactiveUI;

namespace Studio.ViewModels.Workspace.Objects
{
    public class ResourceValidationObject : ReactiveObject
    {
        /// <summary>
        /// Name of this object
        /// </summary>
        public string DecoratedName
        {
            get => _decoratedName;
            set => this.RaiseAndSetIfChanged(ref _decoratedName, value);
        }

        /// <summary>
        /// Identifier of this object, immutable
        /// </summary>
        public uint ID { get; set; }
        
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
            Instances.Add(instance);
            _unique.Add(message, instance);
        }
        
        /// <summary>
        /// Unique lookup table
        /// </summary>
        private Dictionary<string, ResourceValidationInstance> _unique = new();

        /// <summary>
        /// Internal name
        /// </summary>
        private string _decoratedName = string.Empty;
    }
}