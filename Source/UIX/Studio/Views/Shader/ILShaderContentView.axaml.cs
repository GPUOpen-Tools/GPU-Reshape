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
using System.Reactive.Disposables;
using Avalonia.Controls;
using AvaloniaEdit.TextMate;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.Utils.Workspace;
using Runtime.ViewModels.IL;
using Studio.Extensions;
using Studio.Models.Instrumentation;
using Studio.Models.Workspace.Objects;
using Studio.Services;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;
using Studio.Views.Editor;

namespace Studio.Views.Shader
{
    public partial class ILShaderContentView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel { get => DataContext; set => DataContext = value; }
        
        public ILShaderContentView()
        {
            InitializeComponent();

            // Create the registry
            var registryOptions = new TextMate.RegistryOptions();

            // Create a text mate instance
            var textMate = Editor.InstallTextMate(registryOptions);

            // Set the default grammar (just assume .hlsl)
            textMate.SetGrammar("IL");

            // Create background renderer
            _validationBackgroundRenderer = new ValidationBackgroundRenderer
            {
                Document = Editor.Document
            };

            // Create marker service
            _validationTextMarkerService = new ValidationTextMarkerService
            {
                Document = Editor.Document
            };

            // Configure marker canvas
            MarkerCanvas.TextView = Editor.TextArea.TextView;

            // Add renderers
            Editor.TextArea.TextView.BackgroundRenderers.Add(_validationBackgroundRenderer);
            Editor.TextArea.TextView.BackgroundRenderers.Add(_validationTextMarkerService);
            Editor.TextArea.TextView.LineTransformers.Add(_validationTextMarkerService);

            // Add services
            var services = AvaloniaEdit.Utils.ServiceExtensions.GetService<AvaloniaEdit.Utils.IServiceContainer>(Editor.Document);
            services?.AddService(typeof(ValidationTextMarkerService), _validationTextMarkerService);
            
            // Common styling
            Editor.Options.IndentationSize = 4;
            
            // Bind contents
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<ILShaderContentViewModel>()
                .Subscribe(ilViewModel =>
            {
                // Install the extensions
                ServiceRegistry.Get<IEditorService>()?.InstallView(ilViewModel, Editor);
                
                // Update services
                _validationTextMarkerService.ShaderContentViewModel = ilViewModel;
                _validationBackgroundRenderer.ShaderContentViewModel = ilViewModel;
                MarkerCanvas.ShaderContentViewModel = ilViewModel;
                
                // Bind detail
                ilViewModel.MarkerCanvasViewModel.DetailCommand = ReactiveCommand.Create<ITextualSourceObject>(OnDetailCommand);

                // Assign marker view model
                MarkerCanvas.DataContext = ilViewModel.MarkerCanvasViewModel;
                
                // Bind assembled data
                ilViewModel.WhenAnyValue(x => x.AssembledProgram).WhereNotNull().Subscribe(assembled =>
                {
                    // Set text
                    Editor.Text = assembled;
                    
                    // Push all pending objects
                    _pendingAssembling.ForEach(x => OnValidationObjectAdded(ilViewModel, x));
                    _pendingAssembling.Clear();

                    // Bind navigation location
                    ilViewModel.WhenAnyValue(y => y.NavigationLocation)
                        .WhereNotNull()
                        .Subscribe(location => UpdateNavigationLocation(ilViewModel, location!));
                    
                    // Invalidate marker layout
                    MarkerCanvas.UpdateLayout();
                });
                
                // Bind object model
                // We're working with individual shaders here
                ilViewModel.WhenAnyValue(y => y.Content).CastNullable<ShaderViewModel>().WhereNotNull().Subscribe(_object =>
                {
                    // Bind objects
                    _object.ValidationObjects.ToObservableChangeSet()
                        .AsObservableList()
                        .Connect()
                        .OnItemAdded(x => OnValidationObjectAdded(ilViewModel, x))
                        .OnItemRemoved(x => OnValidationObjectRemoved(ilViewModel, x))
                        .Subscribe();
                    
                    // Bind status
                    _object.WhenAnyValue(o => o.AsyncStatus).Subscribe(status =>
                    {
                        if (status.HasFlag(AsyncObjectStatus.NotFound))
                        {
                            Editor.Text = Studio.Resources.Resources.Shader_NotFound;
                        }
                    });
                });
                
                // Reset front state
                ilViewModel.SelectedTextualSourceObject = null;
                ilViewModel.DetailViewModel = null;
            });
        }

        /// <summary>
        /// Invoked on detailed requests
        /// </summary>
        private void OnDetailCommand(ITextualSourceObject sourceObject)
        {
            // Validation
            if (DataContext is not ILShaderContentViewModel
                {
                    Content: ShaderViewModel shaderViewModel, 
                    PropertyCollection: {} property
                } vm)
            {
                return;
            }

            InstrumentationVersion? version = null;

            // If validation, bind instrumentation
            if (sourceObject is ValidationObject validationObject)
            {
                // Check if there's any detailed info at all
                if (!ShaderDetailUtils.CanDetailCollect(validationObject, shaderViewModel))
                {
                    vm.DetailViewModel = new NoDetailViewModel
                    {
                        Object = vm.Content as ShaderViewModel,
                        PropertyCollection = vm.PropertyCollection
                    };
                    return;
                }
                
                // Ensure detailed collection has started
                version = ShaderDetailUtils.BeginDetailedCollection(shaderViewModel, property);
            }

            // Set selection
            vm.SelectedTextualSourceObject = sourceObject;
            
            // Set global selection
            if (ServiceRegistry.Get<ISourceService>() is { } sourceService)
            {
                sourceService.SelectedSourceObject = sourceObject;
            }
            
            // Bind detail context
            sourceObject.WhenAnyValue(x => x.DetailViewModel).Subscribe(x =>
            {
                vm.DetailViewModel = x ?? new MissingDetailViewModel()
                {
                    Object = vm.Content as ShaderViewModel,
                    PropertyCollection = vm.PropertyCollection,
                    Version = version
                };
            }).DisposeWithClear(_detailDisposable);
        }

        private void UpdateNavigationLocation(ILShaderContentViewModel il, NavigationLocation location)
        {
            // Get assembled mapping
            AssembledLineMapping? mapping = il.Assembler?.GetLineMapping(location.Location.BasicBlockId, location.Location.InstructionIndex);
            if (mapping == null)
            {
                return;
            }

            // Update selected file
            il.SelectedTextualSourceObject = location.Object;
                            
            // Scroll to target
            // TODO: 10 is a total guess, we need to derive it from the height, but that doesn't exist yet.
            //       Probably a delayed action?
            Editor.TextArea.Caret.Line = (int)mapping.Value.Line - 10;
            Editor.TextArea.Caret.Column = 0;
            Editor.TextArea.Caret.BringCaretToView();
                    
            // Invalidate marker layout
            MarkerCanvas.UpdateLayout();
        }
        
        /// <summary>
        /// Invoked on object added
        /// </summary>
        /// <param name="validationObject"></param>
        private void OnValidationObjectAdded(ILShaderContentViewModel viewModel, ValidationObject validationObject)
        {
            // Pending assembling?
            if (DataContext is ILShaderContentViewModel { Assembler: null })
            {
                _pendingAssembling.Add(validationObject);
                return;
            }
            
            // Update services
            _validationTextMarkerService.Add(validationObject);
            _validationBackgroundRenderer.Add(validationObject);
            
            // Update canvas
            viewModel.MarkerCanvasViewModel.SourceObjects.Add(validationObject);
            
            // Redraw for background update
            Editor.TextArea.TextView.Redraw();
        }

        /// <summary>
        /// Invoked on object removed
        /// </summary>
        /// <param name="validationObject"></param>
        private void OnValidationObjectRemoved(ILShaderContentViewModel viewModel, ValidationObject validationObject)
        {
            // Update services
            _validationTextMarkerService.Remove(validationObject);
            _validationBackgroundRenderer.Remove(validationObject);
            
            // Update canvas
            viewModel.MarkerCanvasViewModel.SourceObjects.Remove(validationObject);
            
            // Redraw for background update
            Editor.TextArea.TextView.Redraw();
        }

        /// <summary>
        /// Background rendering
        /// </summary>
        private ValidationBackgroundRenderer _validationBackgroundRenderer;

        /// <summary>
        /// Text marker service, hosts transformed objects
        /// </summary>
        private ValidationTextMarkerService _validationTextMarkerService;

        /// <summary>
        /// All pending assembling objects
        /// </summary>
        private List<ValidationObject> _pendingAssembling = new();
        
        /// <summary>
        /// Disposable for detailed data
        /// </summary>
        private CompositeDisposable _detailDisposable = new();
    }
}