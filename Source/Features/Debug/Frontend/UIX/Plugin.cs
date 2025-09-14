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
using DynamicData;
using GRS.Features.Debug.UIX.Settings;
using GRS.Features.Debug.UIX.ViewModels;
using GRS.Features.Debug.UIX.ViewModels.Editor;
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

namespace GRS.Features.Debug.UIX
{
    public class Plugin : IPlugin, IWorkspaceExtension, IEditorExtension
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
            ServiceRegistry.Add(new BreakpointDisplayRegistryService());
            
            // Install the settings
            ServiceRegistry.Get<ISettingsService>()?.Add(new DebugSettingViewModel());
            
            // Add locators
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(DebugSettingViewModel), typeof(DebugSettingView));
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(BreakpointViewModel), typeof(BreakpointDisplayView));
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(BreakpointViewModel), typeof(BreakpointWindow), ViewType.Window);
            
            // Display locators
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(ImageBreakpointDisplayViewModel), typeof(ImageBreakpointDisplayView));
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(ImageBreakpointDisplayViewModel), typeof(ImageBreakpointDisplayConfigView), ViewType.Config);
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(ImageBreakpointDisplayViewModel), typeof(ImageBreakpointDisplayStatusView), ViewType.Status);
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(ImageBreakpointDisplayViewModel), typeof(ImageBreakpointDisplayOverlayView), ViewType.Overlay);
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(LooseBreakpointDisplayViewModel), typeof(LooseBreakpointDisplayView));
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(LooseBreakpointDisplayViewModel), typeof(LooseBreakpointDisplayConfigView), ViewType.Config);

            // Register context actions
            ServiceRegistry.Get<IContextMenuService>()?.ViewModels.AddRange([
                new BreakpointContextViewModel()
            ]);

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
            public ITextualShaderContentViewModel? ShaderContentViewModel { get; set; }

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
        public void InstallViewModel(IShaderContentViewModel viewModel)
        {
            // Must be textual, ignore diagrams
            if (viewModel is not ITextualShaderContentViewModel textualShaderViewModel)
            {
                return;
            }

            // The actual shader and property may change, bind them
            viewModel
                .WhenAnyValue(x => x.PropertyCollection, y => y.ShaderViewModel)
                .Where(x => x is { Item1: not null, Item2: not null })
                .Subscribe(_ =>
            {
                // Try to get the breakpoint collection
                if (BreakpointUtils.GetShaderBreakpointCollection(viewModel.PropertyCollection!, viewModel.ShaderViewModel!) is not { } collectionViewModel)
                {
                    return;
                }
                
                // Check if we have the breakpoint service
                // We need to dynamically rebind them, so it's unfortunately not that simple
                if (collectionViewModel.GetServiceWhere<ShaderContentBreakpointServiceViewModel>(x =>
                    x.ContentViewModel == textualShaderViewModel && x.BreakpointCollectionViewModel == collectionViewModel
                ) is null)
                {
                    // Create service
                    ShaderContentBreakpointServiceViewModel breakpointService = new()
                    {
                        ContentViewModel = textualShaderViewModel,
                        BreakpointCollectionViewModel = collectionViewModel
                    };
            
                    // Bind and keep track of it
                    breakpointService.Bind();
                    viewModel.Services.Add(breakpointService);
                }
            });
        }

        /// <summary>
        /// Install extensions against an editor
        /// </summary>
        public void InstallView(IShaderContentViewModel viewModel, TextEditor textEditor)
        {
            // Must be textual, ignore diagrams
            if (viewModel is not ITextualShaderContentViewModel textualShaderViewModel)
            {
                return;
            }
            
            // Try to get the breakpoint collection
            if (BreakpointUtils.GetShaderBreakpointCollection(viewModel.PropertyCollection!, viewModel.ShaderViewModel!) is not { } collectionViewModel)
            {
                return;
            }
            
            // Add background renderer
            textEditor.TextArea.TextView.BackgroundRenderers.Add(new ValidationTextMarkerService());
            
            // Add breakpoint margin
            textEditor.TextArea.LeftMargins.Insert(0, new BreakpointMargin
            {
                ContextMenu = textEditor.ContextMenu,
                DataContext = new BreakpointMarginViewModel
                {
                    ContentViewModel = textualShaderViewModel,
                    CollectionViewModel = collectionViewModel
                }
            });
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
            // Add breakpoint registry
            workspaceViewModel.PropertyCollection.Services.Add(new BreakpointRegistryService()
            {
                WorkspaceViewModel = workspaceViewModel
            });
            
            // Create service
            workspaceViewModel.PropertyCollection.Services.Add(new DebugService(workspaceViewModel));
        }
    }
}
