using System;
using System.Collections.Generic;
using System.Diagnostics;
using Avalonia.Threading;

namespace Runtime.Threading
{
    public static class GlobalBus
    {
        static GlobalBus()
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