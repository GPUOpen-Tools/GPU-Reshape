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
using System.Reactive.Linq;
using Avalonia;
using Dock.Model.Core;
using Dock.Model.Core.Events;
using Studio.ViewModels.Documents;
using Dock.Model.ReactiveUI.Controls;
using ReactiveUI;
using Runtime.ViewModels;
using Studio.Extensions;
using Studio.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Docks
{
    public class StandardDocumentDock : DocumentDock, IDocumentLayoutViewModel
    {
        public StandardDocumentDock()
        {
            // Bind bottom
            CreateDocument = ReactiveCommand.Create<IDescriptor>(OpenDocument);

            // Bind factory
            this.WhenAnyValue(x => x.Factory)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Bind erased events
                    x.Events().DockableClosed.WhereNotNull().Subscribe(dock => OnDockableErased(dock));
                    x.Events().DockableRemoved.WhereNotNull().Subscribe(dock => OnDockableErased(dock));
                });
        }

        /// <summary>
        /// Invoked on document erasure
        /// </summary>
        private void OnDockableErased(IDockable dock)
        {
            // May not be tracked
            if (!_dockables.TryGetValue(dock, out object? identifier))
            {
                return;
            }
                        
            // Remove relations
            _dockables.Remove(dock);
            _identifiers.Remove(identifier);
        }

        /// <summary>
        /// Open a new document
        /// </summary>
        public void OpenDocument(IDescriptor descriptor)
        {
            // Valid state?
            if (descriptor.Identifier == null)
            {
                return;
            }

            // If the document already exists, simply make it focused
            if (_identifiers.TryGetValue(descriptor.Identifier, out IDockable? existingDockable))
            {
                Factory?.SetActiveDockable(existingDockable);
                Factory?.SetFocusedDockable(this, existingDockable);
                
                // Reassign descriptor if possible
                if (existingDockable is IDocumentViewModel { } existingDocumentViewModel)
                {
                    existingDocumentViewModel.Descriptor = descriptor;
                }
                return;
            }
            
            // Attempt to instantiate
            IDockable? dockable = _locator?.InstantiateDerived<IDockable>(descriptor);
            if (dockable == null)
            {
                return;
            }

            // Assign descriptor if possible
            if (dockable is IDocumentViewModel { } documentViewModel)
            {
                documentViewModel.Descriptor = descriptor;
            }
            
            // Add and set focus
            Factory?.AddDockable(this, dockable);
            Factory?.SetActiveDockable(dockable);
            Factory?.SetFocusedDockable(this, dockable);

            // Add to map
            _identifiers.Add(descriptor.Identifier, dockable);
            _dockables.Add(dockable, descriptor.Identifier);
        }

        /// <summary>
        /// Close all documents with a specific ownership
        /// </summary>
        /// <param name="owner"></param>
        public void CloseOwnedDocuments(object? owner)
        {
            // Local bucket
            List<Tuple<IDockable, object>> bucket = new();
            
            // Determine dockables to be closed
            foreach ((IDockable dockable, object identifier) in _dockables)
            {
                // Not required to be known
                if (dockable is not IDocumentViewModel { } documentViewModel)
                {
                    continue;
                }

                // Matching dockable?
                if (documentViewModel.Descriptor?.Owner == owner)
                {
                    bucket.Add(Tuple.Create(dockable, identifier));
                }
            }
            
            // Finally, clean up with safe iteration
            foreach (var tuple in bucket)
            {
                Factory?.RemoveDockable(tuple.Item1, false);
            }
        }

        /// <summary>
        /// All tracked dockables
        /// </summary>
        private Dictionary<object, IDockable> _identifiers = new();

        /// <summary>
        /// All tracked dockables
        /// </summary>
        private Dictionary<IDockable, object> _dockables = new();

        /// <summary>
        /// Locator service
        /// </summary>
        private ILocatorService? _locator = App.Locator.GetService<ILocatorService>();
    }
}
