// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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

using System;
using System.Collections.Generic;
using Studio.ViewModels.Workspace.Objects;

namespace Runtime.Threading
{
    public class CollectionMergePumpBus<T>
    {
        /// <summary>
        /// Collection merge event
        /// </summary>
        public Action<IEnumerable<T>>? CollectionMerge;
        
        /// <summary>
        /// Constructor
        /// </summary>
        public CollectionMergePumpBus()
        {
            _bus = new()
            {
                Merge = Merge
            };
        }
        
        /// <summary>
        /// Add a new action, with a unique merge value
        /// </summary>
        public void Add(Action action, T mergeValue)
        {
            // Append unique value
            lock (this)
            {
                if (!_entrySet.Contains(mergeValue))
                {
                    _entrySet.Add(mergeValue);
                    _entries.Add(mergeValue);
                }
            }
            
            // Add to bus
            _bus.Add(action);
        }

        /// <summary>
        /// Invoked on merges
        /// </summary>
        private void Merge()
        {
            T[] copy;
            lock (this)
            {
                // Copy entry list
                copy = new T[_entries.Count];
                _entries.CopyTo(copy);
                
                // Cleanup
                _entrySet.Clear();
                _entries.Clear();
            }
            
            // Handle merges
            CollectionMerge?.Invoke(copy);
        }

        /// <summary>
        /// Underlying bus
        /// </summary>
        private MergePumpBus _bus;

        /// <summary>
        /// Unique set
        /// </summary>
        private HashSet<T> _entrySet = new();
        
        /// <summary>
        /// Entries to merge
        /// </summary>
        private List<T> _entries = new();
    }
}