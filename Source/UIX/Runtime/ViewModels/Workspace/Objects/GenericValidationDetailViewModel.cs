// 
// The MIT License (MIT)
// 
// Copyright (c) 2024 Advanced Micro Devices, Inc.,
// Fatalist Development AB (Avalanche Studio Group),
// and Miguel Petersen.
// 
// All Rights Reserved.
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
using Runtime.Threading;
using Runtime.ViewModels.Traits;

namespace Studio.ViewModels.Workspace.Objects
{
    public class GenericValidationDetailViewModel : ReactiveObject, IValidationDetailViewModel, ISerializable
    {
        /// <summary>
        /// Is the collection paused?
        /// </summary>
        public bool Paused = false;

        /// <summary>
        /// All instances associated to this object
        /// </summary>
        public ObservableCollection<string> Instances { get; } = new();

        public GenericValidationDetailViewModel()
        {
            // Bind pump
            pump.Bind(Instances);
        }

        /// <summary>
        /// Add a unique instance, only filtered against other invocations of AddUniqueInstance
        /// </summary>
        public void AddUniqueInstance(string message)
        {
            // Reject if paused
            if (Paused)
            {
                return;
            }
            
            // Existing?
            if (_unique.Contains(message))
            {
                return;
            }

            // Add to trackers
            pump.Insert(0, message);
            _unique.Add(message);
            
            // Trim
            while (pump.Count > MaxInstances)
            {
                _unique.Remove(pump[^1]);
                pump.RemoveAt(pump.Count - 1);
            }
        }

        /// <summary>
        /// Serialize this object
        /// </summary>
        public object Serialize()
        {
            return new SerializationMap()
            {
                { "Type", "GenericList" },
                { "MaxInstances", MaxInstances },
                { "Instances", new SerializationList(Instances) }
            };
        }

        /// <summary>
        /// Max number of instances
        /// </summary>
        private static int MaxInstances = 1024;
        
        /// <summary>
        /// Unique lookup table
        /// </summary>
        private HashSet<string> _unique = new();

        /// <summary>
        /// Pump for main collection
        /// </summary>
        private PumpedList<string> pump = new();
    }
}