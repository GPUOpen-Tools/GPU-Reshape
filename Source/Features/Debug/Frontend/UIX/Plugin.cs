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
using System.Reactive.Linq;
using Avalonia.Media;
using AvaloniaEdit;
using AvaloniaEdit.Document;
using AvaloniaEdit.Rendering;
using Dock.Model.Core;
using DynamicData;
using GRS.Features.Debug.UIX.Settings;
using GRS.Features.Debug.UIX.ViewModels;
using GRS.Features.Debug.UIX.ViewModels.Editor;
using GRS.Features.Debug.UIX.ViewModels.Tools;
using GRS.Features.Debug.UIX.ViewModels.Utils;
using GRS.Features.Debug.UIX.Workspace;
using ReactiveUI;
using Studio.Plugin;
using Studio.Services;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;
using Studio.Views;
using Studio.Views.Setting;
using UIX.Views;
using UIX.Views.Display;
using UIX.Views.Editor;
using UIX.Views.Tools;

namespace GRS.Features.Debug.UIX
{
    public class Plugin : IPlugin, IWorkspaceExtension, IEditorExtension, IDockingExtension
    {
        public PluginInfo Info { get; } = new()
        {
            Name = "Debug",
            Description = "--",
            Dependencies = new string[]{ }
        };
        
        /// <summary>
        /// Install this plugin
        /// </summary>
        /// <returns></returns>
        public bool Install()
        {
            // Get workspace service
            var workspaceService = ServiceRegistry.Get<IWorkspaceService>();
            
            // Add workspace extension
            workspaceService?.Extensions.Add(this);
            
            // Add editor extension
            ServiceRegistry.Get<IEditorService>()?.Extensions.Add(this);
            
            // Install the archetype registry
            ServiceRegistry.Add(new WatchpointDisplayRegistryService());
            
            // Install the settings
            ServiceRegistry.Get<ISettingsService>()?.Add(new DebugSettingViewModel());
            
            // Install the watchpoint service
            ServiceRegistry.Add(new WatchpointService());
            
            // Add locators
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(DebugSettingViewModel), typeof(DebugSettingView));
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(WatchpointViewModel), typeof(WatchpointDisplayView));
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(WatchpointViewModel), typeof(WatchpointWindow), ViewType.Window);
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(WatchpointTreeViewModel), typeof(WatchpointTreeView));
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(WatchpointStackViewModel), typeof(WatchpointStackView));
            
            // Display locators
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(ImageWatchpointDisplayViewModel), typeof(ImageWatchpointDisplayView));
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(ImageWatchpointDisplayViewModel), typeof(ImageWatchpointDisplayConfigView), ViewType.Config);
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(ImageWatchpointDisplayViewModel), typeof(ImageWatchpointDisplayStatusView), ViewType.Status);
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(ImageWatchpointDisplayViewModel), typeof(ImageWatchpointDisplayOverlayView), ViewType.Overlay);
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(LooseWatchpointDisplayViewModel), typeof(LooseWatchpointDisplayView));
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(LooseWatchpointDisplayViewModel), typeof(LooseWatchpointDisplayConfigView), ViewType.Config);

            // Register context actions
            ServiceRegistry.Get<IContextMenuService>()?.ViewModels.AddRange([
                new WatchpointContextViewModel()
            ]);
            
            // Register docking extensions
            ServiceRegistry.Get<IDockingService>()?.Extensions.Add(this);

            // OK
            return true;
        }
        
        // TODO[dbg]: Move this
        public class ValidationTextMarkerService : DocumentColorizingTransformer, IBackgroundRenderer
        {
            public TextDocument? Document { get; set; }
            
            /// <summary>
            /// Current content view model
            /// </summary>
            public ITextualContent? ShaderContentViewModel { get; set; }

            /// <summary>
            /// Invoked on line draws / colorization 
            /// </summary>
            protected override void ColorizeLine(DocumentLine line)
            {
                
            }

            /// <summary>
            /// Invoked on document drawing
            /// </summary>
            public void Draw(TextView textView, DrawingContext drawingContext)
            {
                
            }

            public KnownLayer Layer { get; }
        }

        /// <summary>
        /// Install all view model extensions
        /// </summary>
        public void InstallViewModel(IContentViewModel viewModel)
        {
            // Must be textual, ignore diagrams
            if (viewModel is not ITextualContent textualShaderViewModel)
            {
                return;
            }
            
            // The actual shader and property may change, bind them
            viewModel
                .WhenAnyValue(x => x.PropertyCollection, y => y.Content)
                .Where(x => x is { Item1: not null, Item2: not null })
                .Subscribe(_ =>
                {
                    // Try to get the watchpoint collection
                    if (WatchpointUtils.GetShaderWatchpointCollection(viewModel.PropertyCollection!, viewModel.Content!) is not { } collectionViewModel)
                    {
                        return;
                    }
                
                    // Check if we have the watchpoint service
                    // We need to dynamically rebind them, so it's unfortunately not that simple
                    if (collectionViewModel.GetServiceWhere<ContentWatchpointServiceViewModel>(x =>
                            x.Content == textualShaderViewModel && 
                            x.WatchpointCollectionViewModel == collectionViewModel
                        ) is null)
                    {
                        // Create service
                        ContentWatchpointServiceViewModel watchpointService = new()
                        {
                            Content = textualShaderViewModel,
                            WatchpointCollectionViewModel = collectionViewModel
                        };
            
                        // Bind and keep track of it
                        watchpointService.Bind();
                        viewModel.Services.Add(watchpointService);
                    }
            });
        }

        /// <summary>
        /// Install extensions against an editor
        /// </summary>
        public void InstallView(IContentViewModel viewModel, TextEditor textEditor)
        {
            // Must be textual, ignore diagrams
            if (viewModel is not ITextualContent textualShaderViewModel)
            {
                return;
            }
            
            // Try to get the watchpoint collection
            if (WatchpointUtils.GetShaderWatchpointCollection(viewModel.PropertyCollection!, viewModel.Content!) is not { } collectionViewModel)
            {
                return;
            }
            
            // Add background renderer
            textEditor.TextArea.TextView.BackgroundRenderers.Add(new ValidationTextMarkerService());
            
            // Add watchpoint margin
            textEditor.TextArea.LeftMargins[0] = new WatchpointMargin(textEditor.TextArea.LeftMargins[0])
            {
                ContextMenu = textEditor.ContextMenu,
                DataContext = new WatchpointMarginViewModel
                {
                    Content = textualShaderViewModel,
                    CollectionViewModel = collectionViewModel
                }
            };
        }

        /// <summary>
        /// Install extensions against the dock layout
        /// </summary>
        public IDockable[] Install(DockingSlot slot)
        {
            switch (slot)
            {
                case DockingSlot.Right:
                    return [new WatchpointTreeViewModel()];
                case DockingSlot.BottomRight:
                    return [new WatchpointStackViewModel()];
            }
            
            return Array.Empty<IDockable>();
        }

        /// <summary>
        /// Uninstall this plugin
        /// </summary>
        public void Uninstall()
        {
            // Remove workspace extension
            ServiceRegistry.Get<IWorkspaceService>()?.Extensions.Remove(this);
        }

        /// <summary>
        /// Install an extension
        /// </summary>
        public void Install(IWorkspaceViewModel workspaceViewModel)
        {
            // Add watchpoint registry
            workspaceViewModel.PropertyCollection.Services.Add(new WatchpointRegistryService()
            {
                WorkspaceViewModel = workspaceViewModel
            });
            
            // Add view model collection registry
            workspaceViewModel.PropertyCollection.Properties.Add(new WatchpointCollectionRegistryViewModel()
            {
                Parent = workspaceViewModel.PropertyCollection
            });
            
            // Create service
            workspaceViewModel.PropertyCollection.Services.Add(new DebugService(workspaceViewModel));
        }
    }
}
