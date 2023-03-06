using System;
using System.Collections.Generic;
using System.Reactive.Linq;
using Avalonia;
using Dock.Model.Core;
using Dock.Model.Core.Events;
using Studio.ViewModels.Documents;
using Dock.Model.ReactiveUI.Controls;
using ReactiveUI;
using Studio.Extensions;
using Studio.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Docks
{
    public class StandardDocumentDock : DocumentDock
    {
        public StandardDocumentDock()
        {
            CreateDocument = ReactiveCommand.Create<IDescriptor>(OnCreateDocument);

            // Bind factory
            this.WhenAnyValue(x => x.Factory)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Bind closed events
                    x.Events().DockableClosed.WhereNotNull().Subscribe(dock =>
                    {
                        if (!_dockables.TryGetValue(dock, out object? identifier))
                        {
                            return;
                        }
                        
                        // Remove relation
                        _dockables.Remove(dock);
                        _identifiers.Remove(identifier);
                    });
                });
        }

        private void OnCreateDocument(IDescriptor descriptor)
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
