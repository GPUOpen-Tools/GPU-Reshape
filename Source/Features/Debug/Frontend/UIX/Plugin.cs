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

using Avalonia.Media;
using AvaloniaEdit;
using AvaloniaEdit.Document;
using AvaloniaEdit.Rendering;
using DynamicData;
using GRS.Features.Debug.UIX.Settings;
using GRS.Features.Debug.UIX.ViewModels;
using GRS.Features.Debug.UIX.Workspace;
using Runtime.Models.Objects;
using Runtime.ViewModels.Traits;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Models.Workspace;
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
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(BreakpointViewModel), typeof(BreakpointWindow));
            
            // Display locators
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(ImageBreakpointDisplayViewModel), typeof(ImageBreakpointDisplayView));
            ServiceRegistry.Get<ILocatorService>()?.AddDerived(typeof(ImageBreakpointDisplayViewModel), typeof(ImageBreakpointDisplayConfigView), ViewType.Config);

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
        /// Install all code editor extensions
        /// </summary>
        public void InstallView(IShaderContentViewModel viewModel, TextEditor textEditor)
        {
            // Must be textual, ignore diagrams
            if (viewModel is not ITextualShaderContentViewModel textualShaderViewModel)
            {
                return;
            }
            
            // Find the debug feature
            FeatureInfo? featureInfo = viewModel.PropertyCollection?
                .GetWorkspaceCollection()?
                .GetWorkspaceCollection()?
                .GetProperty<IFeatureCollectionViewModel>()?
                .GetFeature("Debug");
            
            // TODO[dbg]: Standardize this, and only when breakpoints are added
            
            // Must have shader collection
            var shaderCollectionViewModel = viewModel.PropertyCollection?.GetProperty<IShaderCollectionViewModel>();
            if (shaderCollectionViewModel == null)
            {
                return;
            }
            
            // Find or create shader property
            var shaderViewModel = shaderCollectionViewModel.GetPropertyWhere<Studio.ViewModels.Workspace.Properties.Instrumentation.ShaderViewModel>(x => x.Shader.GUID == viewModel.ShaderViewModel.GUID);
            if (shaderViewModel == null)
            {
                shaderCollectionViewModel.Properties.Add(shaderViewModel = new Studio.ViewModels.Workspace.Properties.Instrumentation.ShaderViewModel()
                {
                    Parent = shaderCollectionViewModel,
                    ConnectionViewModel = shaderCollectionViewModel.ConnectionViewModel,
                    Shader = new ShaderIdentifier()
                    {
                        GUID = viewModel.ShaderViewModel.GUID,
                        Descriptor = $"Shader {viewModel.ShaderViewModel.GUID} - {System.IO.Path.GetFileName(viewModel.ShaderViewModel.Filename)}"
                    }
                });
            }

            // Get breakpoint collection for the shader
            var service = shaderViewModel.GetProperty<ShaderBreakpointCollectionViewModel>();
            if (service == null)
            {
                shaderViewModel.Properties.Add(service = new ShaderBreakpointCollectionViewModel()
                {
                    FeatureInfo = featureInfo!.Value,
                    ShaderProperty = shaderViewModel,
                    Parent = shaderViewModel
                });
            }
            
            // Add background renderer
            textEditor.TextArea.TextView.BackgroundRenderers.Add(new ValidationTextMarkerService());
            
            // Add breakpoint margin
            textEditor.TextArea.LeftMargins.Insert(0, new BreakpointMargin
            {
                ContentViewModel = textualShaderViewModel,
                CollectionViewModel = service
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
