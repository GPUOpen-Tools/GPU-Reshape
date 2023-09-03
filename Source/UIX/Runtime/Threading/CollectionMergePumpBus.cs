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