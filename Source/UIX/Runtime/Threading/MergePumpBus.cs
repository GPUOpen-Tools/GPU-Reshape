using System;

namespace Runtime.Threading
{
    public class MergePumpBus
    {
        /// <summary>
        /// Merge action
        /// </summary>
        public Action? Merge;

        /// <summary>
        /// Add a new action, will post a merge if needed
        /// </summary>
        public void Add(Action action)
        {
            lock (this)
            {
                if (Merge != null && !_merging)
                {
                    GlobalPumpBus.Binding.Post(InternalMergeStart);
                    _merging = true;
                }
            }
            
            GlobalPumpBus.Binding.Post(action);
        }

        /// <summary>
        /// Invoked on merge start
        /// </summary>
        private void InternalMergeStart()
        {
            GlobalPumpBus.Binding.Post(InternalMergeEnd);
        }

        /// <summary>
        /// Invoked on merge end
        /// </summary>
        private void InternalMergeEnd()
        {
            lock (this)
            {
                _merging = false;
            }

            Merge?.Invoke();
        }
        
        /// <summary>
        /// Has a merge been scheduled?
        /// </summary>
        private volatile bool _merging;
    }
}