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
using System.Reactive;
using System.Reactive.Disposables;
using System.Reactive.Linq;
using System.Reactive.Subjects;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Markup.Xaml;
using AvaloniaEdit.TextMate;
using AvaloniaEdit.Utils;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.Utils.Workspace;
using Runtime.ViewModels.IL;
using Runtime.ViewModels.Shader;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;
using Studio.Views.Editor;
using TextMateSharp.Grammars;
using ShaderViewModel = Studio.ViewModels.Documents.ShaderViewModel;

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
            MarkerCanvas.DetailCommand = ReactiveCommand.Create<ValidationObject>(OnDetailCommand);

            // Add renderers
            Editor.TextArea.TextView.BackgroundRenderers.Add(_validationBackgroundRenderer);
            Editor.TextArea.TextView.BackgroundRenderers.Add(_validationTextMarkerService);
            Editor.TextArea.TextView.LineTransformers.Add(_validationTextMarkerService);

            // Add services
            IServiceContainer services = Editor.Document.GetService<IServiceContainer>();
            services?.AddService(typeof(ValidationTextMarkerService), _validationTextMarkerService);
            
            // Common styling
            Editor.Options.IndentationSize = 4;
            
            // Bind contents
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<ILShaderContentViewModel>()
                .Subscribe(ilViewModel =>
            {
                // Update services
                _validationTextMarkerService.ShaderContentViewModel = ilViewModel;
                _validationBackgroundRenderer.ShaderContentViewModel = ilViewModel;
                MarkerCanvas.ShaderContentViewModel = ilViewModel;
                
                // Bind assembled data
                ilViewModel.WhenAnyValue(x => x.AssembledProgram).WhereNotNull().Subscribe(assembled =>
                {
                    // Set text
                    Editor.Text = assembled;
                    
                    // Push all pending objects
                    _pendingAssembling.ForEach(OnValidationObjectAdded);
                    _pendingAssembling.Clear();

                    // Bind navigation location
                    ilViewModel.WhenAnyValue(y => y.NavigationLocation)
                        .WhereNotNull()
                        .Subscribe(location => UpdateNavigationLocation(ilViewModel, location!));
                    
                    // Invalidate marker layout
                    MarkerCanvas.UpdateLayout();
                });
                
                // Bind object model
                ilViewModel.WhenAnyValue(y => y.Object).WhereNotNull().Subscribe(_object =>
                {
                    // Bind objects
                    _object.ValidationObjects.ToObservableChangeSet()
                        .AsObservableList()
                        .Connect()
                        .OnItemAdded(OnValidationObjectAdded)
                        .OnItemRemoved(OnValidationObjectRemoved)
                        .Subscribe();
                    
                    // Bind status
                    _object.WhenAnyValue(o => o.AsyncStatus).Subscribe(status =>
                    {
                        if (status.HasFlag(AsyncShaderStatus.NotFound))
                        {
                            Editor.Text = Studio.Resources.Resources.Shader_NotFound;
                        }
                    });
                });
                
                // Reset front state
                ilViewModel.SelectedValidationObject = null;
                ilViewModel.DetailViewModel = null;
            });
        }

        /// <summary>
        /// Invoked on detailed requests
        /// </summary>
        private void OnDetailCommand(ValidationObject validationObject)
        {
            // Validation
            if (DataContext is not ILShaderContentViewModel
                {
                    Object: {} shaderViewModel, 
                    PropertyCollection: {} property
                } vm)
            {
                return;
            }

            // Check if there's any detailed info at all
            if (!ShaderDetailUtils.CanDetailCollect(validationObject, shaderViewModel))
            {
                vm.DetailViewModel = new NoDetailViewModel()
                {
                    Object = vm.Object,
                    PropertyCollection = vm.PropertyCollection
                };
                return;
            }
            
            // Ensure detailed collection has started
            ShaderDetailUtils.BeginDetailedCollection(shaderViewModel, property);
                
            // Set selection
            vm.SelectedValidationObject = validationObject;
            
            // Bind detail context
            validationObject.WhenAnyValue(x => x.DetailViewModel).Subscribe(x =>
            {
                vm.DetailViewModel = x ?? new MissingDetailViewModel()
                {
                    Object = vm.Object,
                    PropertyCollection = vm.PropertyCollection
                };
            }).DisposeWithClear(_detailDisposable);
        }

        private void UpdateNavigationLocation(ILShaderContentViewModel ilViewModel, NavigationLocation location)
        {
            // Get assembled mapping
            AssembledMapping? mapping = ilViewModel.Assembler?.GetMapping(location.Location.BasicBlockId, location.Location.InstructionIndex);
            if (mapping == null)
            {
                return;
            }

            // Update selected file
            ilViewModel.SelectedValidationObject = location.Object;
                            
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
        private void OnValidationObjectAdded(ValidationObject validationObject)
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
            MarkerCanvas.Add(validationObject);
            
            // Redraw for background update
            Editor.TextArea.TextView.Redraw();
        }

        /// <summary>
        /// Invoked on object removed
        /// </summary>
        /// <param name="validationObject"></param>
        private void OnValidationObjectRemoved(ValidationObject validationObject)
        {
            // Update services
            _validationTextMarkerService.Remove(validationObject);
            _validationBackgroundRenderer.Remove(validationObject);
            
            // Update canvas
            MarkerCanvas.Remove(validationObject);
            
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