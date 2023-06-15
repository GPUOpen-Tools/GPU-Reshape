using System;
using System.Collections.Generic;
using System.Diagnostics;
using DynamicData;

namespace Runtime.Threading
{
    public class PumpedList<T>
    {
        /// <summary>
        /// Pump interval
        /// </summary>
        public TimeSpan Interval = TimeSpan.FromMilliseconds(250);
        
        /// <summary>
        /// Number of items
        /// </summary>
        public int Count => _mirror.Count;

        /// <summary>
        /// Add a new item
        /// </summary>
        /// <param name="value"></param>
        public void Add(T value)
        {
            _mirror.Add(value);
            Schedule();
        }
        
        /// <summary>
        /// Insert an item
        /// </summary>
        /// <param name="index">index to be inserted to</param>
        /// <param name="value"></param>
        public void Insert(int index, T value)
        {
            _mirror.Insert(index, value);
            Schedule();
        }
        
        /// <summary>
        /// Remove an item at a given position
        /// </summary>
        /// <param name="index"></param>
        public void RemoveAt(int index)
        {
            _mirror.RemoveAt(index);
            Schedule();
        }

        /// <summary>
        /// Get an item
        /// </summary>
        /// <param name="index"></param>
        public T this[int index] => _mirror[index];

        /// <summary>
        /// Bind this to a list
        /// </summary>
        /// <param name="target"></param>
        public void Bind(IList<T> target)
        {
            _target = target;
        }

        /// <summary>
        /// Schedule a pending pump
        /// </summary>
        private void Schedule()
        {
            // Check thread specific value
            if (_localPending)
            {
                return;
            }
            
            // Thread has requested an update
            lock (this)
            {
                // Check if the remote, thread-safe, pending state
                if (_remotePending)
                {
                    return;
                }

                // Mark as remote pending
                _remotePending = true;
                
                // Copy data
                _committed = new List<T>(_mirror);
            }
            
            // Schedule
            GlobalBus.Add(Interval, Commit);
        }

        /// <summary>
        /// Commit all pending work
        /// </summary>
        private void Commit()
        {
            lock (this)
            {
                Debug.Assert(_committed != null);

                // Copy all to target
                _target.Clear();
                _target.AddRange(_committed);

                // Cleanup
                _committed = null;
                _remotePending = false;
            }
        }

        /// <summary>
        /// Target list
        /// </summary>
        private IList<T> _target;

        /// <summary>
        /// Thread visible pending state
        /// </summary>
        private volatile bool _localPending = false;

        /// <summary>
        /// Remote pending state
        /// </summary>
        private volatile bool _remotePending = false;
        
        /// <summary>
        /// Internal mirror to target
        /// </summary>
        private List<T> _mirror = new();

        /// <summary>
        /// Committed data
        /// </summary>
        private List<T>? _committed;
    }
}