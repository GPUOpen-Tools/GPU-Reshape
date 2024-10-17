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

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reactive.Disposables;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.ViewModels.Controls;

namespace Studio.ViewModels.Workspace.Message
{
    public class HierarchicalMessageVirtualizationViewModel : ReactiveObject
    {
        /// <summary>
        /// All virtualized items, flattened
        /// </summary>
        public ObservableCollectionExtended<IObservableTreeItem> Items { get; } = new();

        /// <summary>
        /// Virtualize a root item, must be base
        /// </summary>
        public void Virtualize(IVirtualizedObservableTreeItem root)
        {
            using var insertion   = WithInsertion(0);
            using var suspension = WithSuspension();

            // While the root item has children, the node itself never appears, so add a fake indentation
            root.Indentation = -1;
            
            // Keep track of the root for insertion
            _root = root;
            
            // Handle all children
            VirtualizeChildren(root);
        }

        /// <summary>
        /// Add a new item
        /// </summary>
        /// <param name="item">item to be added</param>
        /// <param name="indentation">indentation to assign</param>
        private void Add(IVirtualizedObservableTreeItem item, int indentation)
        {
            // Redundant check in case of collection refires
            if (_itemsSet.Contains(item))
            {
                return;
            }
            
            // Setup
            item.Indentation = indentation;
            
            // Flatten item
            Items.Insert(_scope!.Index++, item);
            _itemsSet.Add(item);
            
            // Handle all children
            VirtualizeChildren(item);
        }

        /// <summary>
        /// Remove an item
        /// </summary>
        /// <param name="index">index to be removed</param>
        private void Remove(int index)
        {
            _itemsSet.Remove(Items[index]);
            Items.RemoveAt(index);
        }

        /// <summary>
        /// Virtualize all children of an item
        /// </summary>
        /// <param name="item">item whose children should be virtualized</param>
        private void VirtualizeChildren(IVirtualizedObservableTreeItem item)
        {
            // If expanded, add all the children
            if (item.IsExpanded)
            {
                AddChildren(item);
            }
            
            // Otherwise we're just subscribing to container events
            // Since we're sharing a disposable for the whole collection, only subscribe once
            if (_subscribed.Contains(item))
            {
                return;
            }

            // Subscription should never handle pending events
            _noFire = true;
            
            // Handle expansion changes
            item.WhenAnyValue(x => x.IsExpanded)
                .Subscribe(_ => OnExpanded(item))
                .DisposeWith(_disposable);
            
            // Handle container changes
            item.Items
                .ToObservableChangeSet()
                .OnItemAdded(child => OnItemAdded(item, child))
                .OnItemRemoved(child => OnItemRemoved(item, child), false)
                .Subscribe()
                .DisposeWith(_disposable);

            // Allow events again
            _noFire = false;
            
            // Mark as subscribed
            _subscribed.Add(item);
        }

        /// <summary>
        /// Invoked on container additions
        /// </summary>
        /// <param name="parent">parent of the item</param>
        /// <param name="item">item to be added</param>
        private void OnItemAdded(IVirtualizedObservableTreeItem parent, IObservableTreeItem item)
        {
            // Check event state
            if (_noFire)
            {
                return;
            }

            // Only handle additions if the parent is expanded
            // And we're not already tracking it (event re-fires)
            if (!parent.IsExpanded || _itemsSet.Contains(item))
            {
                return;
            }

            // Determine where the parent position
            if (!GetInsertionIndex(parent, out int index))
            {
                return;
            }
            
            // Setup scopes
            using var insertion   = WithInsertion(index);
            using var suspension = WithSuspension();
            
            // Index of the child within the parent
            int childIndex = parent.Items.IndexOf(item);

            // Offset insertion index by the existing expanded children and this items position
            for (int i = 0; i < childIndex; i++)
            {
                _scope.Index += GetVirtualizedCount(parent.Items[i]);
            }
            
            // Add it!
            Add((IVirtualizedObservableTreeItem)item, parent.Indentation + 1);
        }

        /// <summary>
        /// Invoked on container removals
        /// </summary>
        /// <param name="parent">parent of the item</param>
        /// <param name="item">item to be removed</param>
        private void OnItemRemoved(IVirtualizedObservableTreeItem parent, IObservableTreeItem item)
        {
            // Check event state
            if (_noFire)
            {
                return;
            }

            // Only handle additions if the parent is expanded
            // And we're already tracking it (event re-fires)
            if (!parent.IsExpanded || !_itemsSet.Contains(item))
            {
                return;
            }
            
            // Find the item to be removed
            int index = Items.IndexOf(item);
            if (index == -1)
            {
                return;
            }

            // Setup scopes
            using var suspension = WithSuspension();
            
            // Determine the virtual count of this item (including children)
            int count = GetVirtualizedCount(item);

            // Remove all
            // TODO: Observable collection doesn't have a ranged removal
            for (int i = 0; i < count; i++)
            {
                Remove(index);
            }
        }

        /// <summary>
        /// Invoked on expansion changes
        /// </summary>
        /// <param name="item">item that changed</param>
        private void OnExpanded(IVirtualizedObservableTreeItem item)
        {
            // Check event state
            if (_noFire)
            {
                return;
            }

            // Setup scopes
            using var suspension = WithSuspension();

            // Determine where the is parent positioned
            int index;
            if (!GetInsertionIndex(item, out index))
            {
                return;
            }

            // Are we expanding?
            if (item.IsExpanded)
            {
                using var insertion = WithInsertion(index);
                
                // Append items after the parent
                AddChildren(item);
            }
            else
            {
                // Determine the number of virtual items, but do not include the parent
                int count = GetVirtualizedCount(item, true) - 1;

                // Remove all excluding parent
                // TODO: Observable collection doesn't have a ranged removal
                for (int i = 0; i < count; i++)
                {
                    Remove(index);
                }
            }
        }

        /// <summary>
        /// Add all children
        /// </summary>
        /// <param name="item">items whose children should be added</param>
        private void AddChildren(IVirtualizedObservableTreeItem item)
        {
            foreach (IObservableTreeItem child in item.Items)
            {
                Add((IVirtualizedObservableTreeItem)child, item.Indentation + 1);
            }
        }
        
        /// <summary>
        /// Get the insertion index for a sub-node for a specific item
        /// </summary>
        /// <param name="item">item whose position should be searched for</param>
        /// <param name="index">destination index</param>
        /// <returns></returns>
        private bool GetInsertionIndex(IObservableTreeItem item, out int index)
        {
            // Edge case, if inserting from the root, report the base (zero)
            // Inter-item dependencies are handled elsewhere (e.g., item 3 in root)
            if (item == _root)
            {
                index = 0;
                return true;
            }
            
            // Linearly search for the item
            index = Items.IndexOf(item);
            if (index == -1)
            {
                return false;
            }

            // Report the next (insertion) position
            index++;
            return true;
        }

        /// <summary>
        /// Get the number of virtual items
        /// </summary>
        /// <param name="item">item to check for</param>
        /// <param name="forceChildren">if true, considers its childrens regardless of expansion states</param>
        /// <returns>count</returns>
        private int GetVirtualizedCount(IObservableTreeItem item, bool forceChildren = false)
        {
            // Consider the item itself
            int count = 1;

            // Only consider children if requested
            if (item.IsExpanded || forceChildren)
            {
                foreach (IObservableTreeItem child in item.Items)
                {
                    count += GetVirtualizedCount(child);
                }
            }

            // Total number of items
            return count;
        }

        /// <summary>
        /// Clear this container
        /// </summary>
        public void Clear()
        {
            _disposable.Clear();
            _subscribed.Clear();
            _itemsSet.Clear();
            Items.Clear();
        }

        /// <summary>
        /// Create an event suspension scope
        /// </summary>
        private IDisposable? WithSuspension()
        {
            // If already suspended, don't do anything
            if (_isSuspended)
            {
                return null;
            }

            // Suspended!
            _isSuspended = true;

            // Ignore any container events until disposal
            var suspension = Items.SuspendNotifications();

            return Disposable.Create(() =>
            {
                _isSuspended = false;
                suspension.Dispose();
            });
        }

        /// <summary>
        /// Create an insertion scope
        /// </summary>
        private IDisposable WithInsertion(int index)
        {
            // Insertion re-entries are not allowed
            // There is at most a single insertion event root
            Debug.Assert(_scope == null);
            
            // Create scope at index
            _scope = new InsertionScope()
            {
                Index = index
            };

            // On disposal, just set it to null
            return Disposable.Create(() =>
            {
                _scope = null;
            });
        }

        /// <summary>
        /// Insertion scope, just an index that's moved
        /// </summary>
        class InsertionScope
        {
            public int Index;
        }

        /// <summary>
        /// Current insertion scope, optional
        /// </summary>
        private InsertionScope? _scope;

        /// <summary>
        /// Are events currently suspended?
        /// </summary>
        private bool _isSuspended = false;

        /// <summary>
        /// Should subscriptions be ignored?
        /// </summary>
        private bool _noFire = false;

        /// <summary>
        /// Root item, one at most
        /// </summary>
        private IObservableTreeItem? _root = null;

        /// <summary>
        /// Set of subscribed observables
        /// </summary>
        private HashSet<IVirtualizedObservableTreeItem> _subscribed = new();

        /// <summary>
        /// Set of items currently tracked
        /// </summary>
        private HashSet<IObservableTreeItem> _itemsSet = new();
        
        /// <summary>
        /// Shared disposable for all events?
        /// </summary>
        private CompositeDisposable _disposable = new();
    }   
}