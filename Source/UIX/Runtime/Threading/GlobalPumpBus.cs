using System;
using System.Collections.Generic;
using System.Diagnostics;
using Avalonia;
using Avalonia.Platform;
using Avalonia.Threading;
using DynamicData;

namespace Runtime.Threading
{
    public class GlobalPumpBus
    {
        /// <summary>
        /// Given priority
        /// </summary>
        public DispatcherPriority Priority;

        /// <summary>
        /// Shared bus
        /// Higher priority than Layout updates, will not re-invoke layout invalidations
        /// </summary>
        public static GlobalPumpBus Binding = new()
        {
            Priority = DispatcherPriority.Normal
        };

        /// <summary>
        /// Post an action
        /// </summary>
        public void Post(Action action)
        {
            lock (this)
            {
                _actions.Add(action);
                EnqueueNoLock();
            }
        }

        /// <summary>
        /// Enqueue a run, does not lock
        /// </summary>
        private void EnqueueNoLock()
        {
            // Already enqueued?
            if (_enqueued)
            {
                return;
            }

            // Mark as enqueued
            _enqueued = true;

            // Set the current slack
            _stepSlackCounter = _stepSlackLimit;
            
            // Enqueue on UI thread
            Dispatcher.UIThread.Post(EnqueueMainThread);
        }

        private void EnqueueMainThread()
        {
            Debug.Assert(Dispatcher.UIThread.CheckAccess(), "Enqueued on foreign thread");
            
            // Dispose last timer
            _disposable?.Dispose();

            // Start new timer
            _disposable = _platformThreading.StartTimer(Priority, TimeSpan.Zero, Run);
        }

        private void Run()
        {
            Action[] chunk;
            lock (this)
            {
                // Handle slack, decrement slack if nothing was in the queue, otherwise restore it
                if (_actions.Count > 0)
                {
                    _stepSlackCounter = _stepSlackLimit;
                }
                else
                {
                    --_stepSlackCounter;
                }
                
                // If we're slacking, re-enqueue, otherwise stop it
                if (_stepSlackCounter > 0)
                {
                    EnqueueMainThread();
                }
                else
                {
                    _disposable?.Dispose();
                    _enqueued = false;
                }
                
                // Copy current chunk
                chunk = new Action[_actions.Count];
                _actions.CopyTo(chunk);
                _actions.Clear();
            }
            
            // Invoke all actions
            foreach (Action action in chunk)
            {
                action.Invoke();
            }
        }

        /// <summary>
        /// Has the timer been enqueued?
        /// </summary>
        private volatile bool _enqueued;

        /// <summary>
        /// Timer disposable
        /// </summary>
        private IDisposable? _disposable;

        /// <summary>
        /// All actions
        /// </summary>
        private List<Action> _actions = new();

        /// <summary>
        /// Default slack limit
        /// </summary>
        private readonly int _stepSlackLimit = 10;

        /// <summary>
        /// Current slack counter
        /// </summary>
        private int _stepSlackCounter;
        
        /// <summary>
        /// Underlying threading interface
        /// </summary>
        private IPlatformThreadingInterface _platformThreading = AvaloniaLocator.Current.GetService<IPlatformThreadingInterface>() 
                                                                 ?? throw new Exception("Missing platform threading");
    }
}