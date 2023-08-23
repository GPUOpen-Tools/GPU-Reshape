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
using AvaloniaEdit.TextMate;
using AvaloniaEdit.Utils;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.Utils.Workspace;
using Runtime.ViewModels.Shader;
using Studio.Extensions;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;
using Studio.Views.Editor;
using TextMateSharp.Grammars;

namespace Studio.Views.Shader
{
    public partial class CodeShaderContentView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel
        {
            get => DataContext;
            set => DataContext = value;
        }

        public CodeShaderContentView()
        {
            InitializeComponent();

            // Create the registry
            var registryOptions = new RegistryOptions(ThemeName.DarkPlus);

            // Create a text mate instance
            var textMate = Editor.InstallTextMate(registryOptions);

            // Set the default grammar (just assume .hlsl)
            textMate.SetGrammar(registryOptions.GetScopeByLanguageId(registryOptions.GetLanguageByExtension(".hlsl").Id));

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
                .WhereNotNull()
                .Cast<CodeShaderContentViewModel>()
                .WhereNotNull()
                .Subscribe(codeViewModel =>
                {
                    // Update services
                    _validationTextMarkerService.ShaderContentViewModel = codeViewModel;
                    _validationBackgroundRenderer.ShaderContentViewModel = codeViewModel;
                    MarkerCanvas.ShaderContentViewModel = codeViewModel;

                    // Bind object model
                    codeViewModel.WhenAnyValue(y => y.Object).WhereNotNull().Subscribe(_object =>
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

                        // Raise navigation changes on file additions
                        _object.FileViewModels.ToObservableChangeSet()
                            .AsObservableList()
                            .Connect()
                            .OnItemAdded(x =>
                            {
                                if (codeViewModel.NavigationLocation != null)
                                {
                                    UpdateNavigationLocation(codeViewModel, codeViewModel.NavigationLocation.Value);
                                }
                            })
                            .Subscribe();
                        
                        // Bind status
                        _object.WhenAnyValue(o => o.AsyncStatus).Subscribe(status =>
                        {
                            if (status.HasFlag(AsyncShaderStatus.NotFound))
                            {
                                Editor.Text = Studio.Resources.Resources.Shader_NotFound;
                            }
                            else if (status.HasFlag(AsyncShaderStatus.NoDebugSymbols))
                            {
                                Editor.Text = Studio.Resources.Resources.Shader_NoDebugSymbols;
                            }
                        });
                    });

                    // Bind selected contents
                    codeViewModel.WhenAnyValue(y => y.SelectedShaderFileViewModel, y => y?.Contents)
                        .WhereNotNull()
                        .Subscribe(contents =>
                        {
                            // Set offset to start
                            Editor.TextArea.Caret.Line = 0;
                            Editor.TextArea.Caret.Column = 0;
                            Editor.TextArea.Caret.BringCaretToView();
                            
                            // Clear and set, avoids internal replacement reformatting hell
                            Editor.Text = string.Empty;
                            Editor.Text = contents;

                            // Invalidate services
                            _validationTextMarkerService.ResumarizeValidationObjects();
                    
                            // Invalidate marker layout
                            MarkerCanvas.UpdateLayout();
                        });

                    // Bind navigation location
                    codeViewModel.WhenAnyValue(y => y.NavigationLocation)
                        .WhereNotNull()
                        .Subscribe(location => UpdateNavigationLocation(codeViewModel, location!.Value));

                    // Reset front state
                    codeViewModel.SelectedValidationObject = null;
                    codeViewModel.DetailViewModel = null;
                });
        }

        /// <summary>
        /// Invoked on detail requests
        /// </summary>
        private void OnDetailCommand(ValidationObject validationObject)
        {
            // Validation
            if (DataContext is not CodeShaderContentViewModel
                {
                    Object: {} shaderViewModel, 
                    PropertyCollection: {} property
                } vm)
            {
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

        private void UpdateNavigationLocation(CodeShaderContentViewModel codeViewModel, ShaderLocation location)
        {
            // Attempt to find file vm
            ShaderFileViewModel? fileViewModel = codeViewModel.Object?.FileViewModels.FirstOrDefault(x => x.UID == location.FileUID);
            if (fileViewModel == null)
            {
                return;
            }

            // Update selected file
            codeViewModel.SelectedShaderFileViewModel = fileViewModel;

            // Scroll to target
            // TODO: 10 is a total guess, we need to derive it from the height, but that doesn't exist yet.
            //       Probably a delayed action?
            Editor.TextArea.Caret.Line = location.Line - 10;
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
            // Update services
            _validationTextMarkerService.Add(validationObject);
            
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
        /// Disposable for detailed data
        /// </summary>
        private CompositeDisposable _detailDisposable = new();
    }
}