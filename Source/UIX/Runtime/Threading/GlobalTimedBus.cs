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

using System;
using System.Collections.Generic;
using System.Diagnostics;
using Avalonia.Threading;

namespace Runtime.Threading
{
    public static class GlobalIntervalBus
    {
        static GlobalIntervalBus()
        {
            // Start watch, potentially hi-res
            Stopwatch.Start();
            
            // Register tick
            Timer.Tick += OnPump;
            
            // Ensure the timer is bound to the ui thread
            Dispatcher.UIThread.InvokeAsync(() => Timer.IsEnabled = true);
        }

        /// <summary>
        /// Add a new event
        /// </summary>
        /// <param name="span"></param>
        /// <param name="action"></param>
        public static void Add(TimeSpan span, Action action)
        {
            lock (Entries)
            {
                Entries.Add(new Entry()
                {
                    Target = Stopwatch.ElapsedMilliseconds + span.Milliseconds,
                    Action = action
                });
            }
        }

        /// <summary>
        /// Pump all events
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private static void OnPump(object? sender, EventArgs e)
        {
            lock (Entries)
            {
                // Current time
                double now = Stopwatch.ElapsedMilliseconds;
                
                // Remove all processed events
                Entries.RemoveAll(entry =>
                {
                    if (now <= entry.Target)
                    {
                        return false;
                    }

                    entry.Action.Invoke();
                    return true;
                });
            }
        }
        
        private struct Entry
        {
            /// <summary>
            /// Target time (ms)
            /// </summary>
            public double Target;
            
            /// <summary>
            /// Action to be invoked
            /// </summary>
            public Action Action;
        }

        /// <summary>
        /// All pending entries
        /// </summary>
        private static readonly List<Entry> Entries = new();

        /// <summary>
        /// Internal timer
        /// </summary>
        private static readonly DispatcherTimer Timer = new()
        {
            Interval = TimeSpan.FromMilliseconds(50)
        };

        /// <summary>
        /// High resolution stopwatch
        /// </summary>
        private static readonly Stopwatch Stopwatch = new();
    }
}