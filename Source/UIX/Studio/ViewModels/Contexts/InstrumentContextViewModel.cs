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
using System.Linq;
using DynamicData;
using ReactiveUI;
using Studio.Extensions;
using Studio.Models.Instrumentation;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;

namespace Studio.ViewModels.Contexts
{
    public class InstrumentContextViewModel : ReactiveObject, IInstrumentContextViewModel
    {
        /// <summary>
        /// Install a context against a target view model
        /// </summary>
        /// <param name="itemViewModels">destination items list</param>
        /// <param name="targetViewModels">the targets to install for</param>
        public void Install(IList<IContextMenuItemViewModel> itemViewModels, object[] targetViewModels)
        {
            if (targetViewModels.PromoteOrNull<IInstrumentableObject>() is not {} instrumentableObjects)
            {
                return;
            }
            
            // Root item
            ContextMenuItemViewModel item = new()
            {
                Header = "Instrument"
            };

            // Contextual actions always work on the same workspace
            IPropertyViewModel? workspaceCollection = instrumentableObjects[0].GetWorkspaceCollection();

            // Get all services for the given view model
            IInstrumentationPropertyService[] services = workspaceCollection?.GetServices<IInstrumentationPropertyService>().ToArray() 
                                                         ?? Array.Empty<IInstrumentationPropertyService>();

            // Create a separate category for all experimental features
            if (services.Any(x => x.Flags.HasFlag(InstrumentationFlag.Experimental)))
            {
                item.Items.Add(new ContextMenuItemViewModel()
                {
                    Header = "Experimental",
                    Items = ConsumeServiceContexts(ref services, instrumentableObjects, x => x.Flags.HasFlag(InstrumentationFlag.Experimental)).ToObservableCollection()
                });
            }

            // List all categorized features after experimental
            item.Items.AddRange(ConsumeServiceContexts(ref services, instrumentableObjects, x => x.Category != string.Empty));
            
            // Standard instrumentation services
            item.Items.Add(new SeparatorContextMenuItemViewModel());
            item.Items.Add(new InstrumentAllContextViewModel());
            item.Items.AddRange(ConsumeServiceContexts(ref services, instrumentableObjects, x => x.Flags.HasFlag(InstrumentationFlag.Standard)));
            
            // Non-Standard instrumentation services
            item.Items.Add(new SeparatorContextMenuItemViewModel());
            item.Items.AddRange(ConsumeServiceContexts(ref services, instrumentableObjects, x => true));
            
            // "None" instrumentation
            item.Items.Add(new SeparatorContextMenuItemViewModel());
            item.Items.Add(new InstrumentNoneContextViewModel());

            // OK
            itemViewModels.Add(item);
        }

        /// <summary>
        /// Consume all service contexts that passes a predicate
        /// </summary>
        private IContextMenuItemViewModel[] ConsumeServiceContexts(ref IInstrumentationPropertyService[] services, IInstrumentableObject[] instrumentableObjects, Func<IInstrumentationPropertyService, bool> predecate)
        {
            List<IContextMenuItemViewModel> items = new();
            
            // Filter source collection
            IInstrumentationPropertyService[] serviceFilter = services.Where(predecate).ToArray();

            // All temporary categories, will not persist in different consumes
            Dictionary<string, IContextMenuItemViewModel> categories = new();
            
            // Filter all items
            foreach (IInstrumentationPropertyService service in serviceFilter)
            {
                IContextMenuItemViewModel item = CreateContextFor(service, instrumentableObjects);
                
                // Categorized?
                if (service.Category != string.Empty)
                {
                    if (!categories.ContainsKey(service.Category))
                    {
                        ContextMenuItemViewModel categoryItem = new()
                        {
                            Header = service.Category
                        };
                        
                        // Keep track of the category
                        categories.Add(service.Category, categoryItem);
                        items.Add(categoryItem);
                    }
                    
                    categories[service.Category].Items.Add(item);
                }
                else
                {
                    items.Add(item);
                }
            }

            // Remove consumed items from source
            services = services.Except(serviceFilter).ToArray();
            return items.ToArray();
        }

        /// <summary>
        /// Create a context item for a given service
        /// </summary>
        private IContextMenuItemViewModel CreateContextFor(IInstrumentationPropertyService service, IInstrumentableObject[] instrumentableObjects)
        {
            ContextMenuItemViewModel itemViewModel = new()
            {
                Header = service.Name,
                IsEnabled = instrumentableObjects.All(service.IsInstrumentationValidFor)
            };

            // Bind command to service
            itemViewModel.Command = ReactiveCommand.Create(async () =>
            {
                foreach (IInstrumentableObject instrumentableObject in instrumentableObjects)
                {
                    // Try to create property
                    if (instrumentableObject.GetOrCreateInstrumentationProperty() is not { } propertyViewModel)
                    {
                        return;
                    }
                
                    // Install against property
                    if (await service.CreateInstrumentationObjectProperty(propertyViewModel, false) is { } instrumentationObjectProperty)
                    {
                        propertyViewModel.Properties.Add(instrumentationObjectProperty);
                    }
                }
            });

            // OK
            return itemViewModel;
        }
    }
}