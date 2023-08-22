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
                });
                
                // Bind object model
                ilViewModel.WhenAnyValue(y => y.Object).WhereNotNull().Subscribe(_object =>
                {
                    // Set objects
                    _validationBackgroundRenderer.ValidationObjects = _object.ValidationObjects;

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

                // Bind navigation location
                ilViewModel.WhenAnyValue(y => y.NavigationLocation)
                    .WhereNotNull()
                    .Subscribe(location => UpdateNavigationLocation(ilViewModel, location!.Value));
                
                // Reset front state
                ilViewModel.SelectedValidationObject = null;
                ilViewModel.IsDetailVisible = false;
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
            
            // Ensure detailed collection has started
            ShaderDetailUtils.BeginDetailedCollection(shaderViewModel, property);
                
            // Mark as visible
            vm.IsDetailVisible = true;

            // Set selection
            vm.SelectedValidationObject = validationObject;
            
            // Bind detail context
            validationObject.WhenAnyValue(x => x.DetailViewModel).Subscribe(x =>
            {
                DetailViewHost.ViewHost.ViewModel = x ?? new MissingDetailViewModel()
                {
                    Object = vm.Object,
                    PropertyCollection = vm.PropertyCollection
                };
            }).DisposeWithClear(_detailDisposable);
        }

        private void UpdateNavigationLocation(ILShaderContentViewModel ilViewModel, ShaderLocation location)
        {
            // Get assembled mapping
            AssembledMapping? mapping = ilViewModel.Assembler?.GetMapping(location.BasicBlockId, location.InstructionIndex);
            if (mapping == null)
            {
                return;
            }
                            
            // Scroll to target
            Editor.TextArea.Caret.Line = (int)mapping.Value.Line;
            Editor.TextArea.Caret.Column = 0;
            Editor.TextArea.Caret.BringCaretToView();
        }
        
        /// <summary>
        /// Invoked on object added
        /// </summary>
        /// <param name="validationObject"></param>
        private void OnValidationObjectAdded(ValidationObject validationObject)
        {
            _validationTextMarkerService.Add(validationObject);
            MarkerCanvas.Add(validationObject);
        }

        /// <summary>
        /// Invoked on object removed
        /// </summary>
        /// <param name="validationObject"></param>
        private void OnValidationObjectRemoved(ValidationObject validationObject)
        {
            _validationTextMarkerService.Remove(validationObject);
            MarkerCanvas.Remove(validationObject);
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
        /// Disposable for detailed data
        /// </summary>
        private CompositeDisposable _detailDisposable = new();
    }
}