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