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
        /// Is the collection paused?
        /// </summary>
        public bool Paused = false;

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
            // Reject if paused
            if (Paused)
            {
                return;
            }
            
            // Existing?
            if (_unique.TryGetValue(message, out ResourceValidationInstance? instance))
            {
                instance.CountNoNotify++;
                return;
            }

            // Create new instance
            instance = new ResourceValidationInstance();
            instance.MessageNoNotify = message;
            
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