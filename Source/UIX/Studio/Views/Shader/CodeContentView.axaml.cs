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
using System.Reactive.Disposables;
using System.Reactive.Linq;
using System.Runtime.CompilerServices;
using Avalonia.Controls;
using Avalonia.Controls.Primitives;
using Avalonia.VisualTree;
using AvaloniaEdit.TextMate;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.Utils.Workspace;
using Runtime.ViewModels.Shader;
using Studio.Extensions;
using Studio.Models.Instrumentation;
using Studio.Models.Workspace.Objects;
using Studio.Services;
using Studio.ViewModels.Code;
using Studio.ViewModels.Shader;
using Studio.ViewModels.Workspace.Objects;
using Studio.Views.Editor;
using TextMateSharp.Grammars;

namespace Studio.Views.Shader
{
    public partial class CodeContentView : UserControl, IViewFor
    {
        /// <summary>
        /// Assigned view model
        /// </summary>
        public object? ViewModel
        {
            get => DataContext;
            set => DataContext = value;
        }

        public CodeContentView()
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

            // Add renderers
            Editor.TextArea.TextView.BackgroundRenderers.Add(_validationBackgroundRenderer);
            Editor.TextArea.TextView.BackgroundRenderers.Add(_validationTextMarkerService);
            Editor.TextArea.TextView.LineTransformers.Add(_validationTextMarkerService);

            // Bind editor events
            Editor.TemplateApplied += OnEditorTemplateApplied;
            
            // Add services
            var services = AvaloniaEdit.Utils.ServiceExtensions.GetService<AvaloniaEdit.Utils.IServiceContainer>(Editor.Document);
            services?.AddService(typeof(ValidationTextMarkerService), _validationTextMarkerService);

            // Common styling
            Editor.Options.IndentationSize = 4;

            // Bind contents
            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Cast<CodeContentViewModel>()
                .WhereNotNull()
                .Subscribe(codeViewModel =>
                {
                    // Install the extensions
                    ServiceRegistry.Get<IEditorService>()?.InstallView(codeViewModel, Editor);

                    // Update services
                    _validationTextMarkerService.ShaderContentViewModel = codeViewModel;
                    _validationBackgroundRenderer.ShaderContentViewModel = codeViewModel;
                    MarkerCanvas.ShaderContentViewModel = codeViewModel;
                    
                    // Bind detail
                    codeViewModel.MarkerCanvasViewModel.DetailCommand = ReactiveCommand.Create<ITextualSourceObject>(OnDetailCommand);
                    
                    // Assign marker view model
                    MarkerCanvas.DataContext = codeViewModel.MarkerCanvasViewModel;
                    
                    // Subscribe to content
                    codeViewModel
                        .WhenAnyValue(y => y.Content)
                        .WhereNotNull()
                        .Subscribe(_ =>
                        {
                            codeViewModel.GetObservableShaders()?
                                .ToObservableChangeSet()
                                .AsObservableList()
                                .Connect()
                                .OnItemAdded(x => OnShaderAdded(codeViewModel, x))
                                .OnItemRemoved(OnShaderRemoved)
                                .Subscribe();
                        });

                    // Bind to the selected file
                    codeViewModel
                        .WhenAnyValue(y => y.SelectedFileViewModel)
                        .WhereNotNull()
                        .Subscribe(file =>
                        {
                            // Remove the last selection
                            _selectionDisposable.Clear();

                            // Create navigation state if missing
                            if (!_navigationStates.TryGetValue(file, out var navigationState))
                            {
                                _navigationStates.Add(file, navigationState = new NavigationState()
                                {
                                    Line = 0,
                                    Column = 0
                                });
                            }

                            _selectedCodeFileViewModel = file;
                            
                            // Bind to contents
                            // Bind to disposable, as we only want to react to a single file
                            file.WhenAnyValue(x => x.Contents)
                                .WhereNotNull()
                                .Subscribe(contents =>
                                {
                                    // Clear and set, avoids internal replacement reformatting hell
                                    Editor.Text = string.Empty;
                                    Editor.Text = contents;

                                    // Set offset to last known position
                                    Editor.TextArea.Caret.Line = navigationState.Line;
                                    Editor.TextArea.Caret.Column = navigationState.Column;
                                    Editor.TextArea.Caret.BringCaretToView();

                                    // Invalidate services
                                    _validationTextMarkerService.ResumarizeValidationObjects();
                    
                                    // Invalidate marker layout
                                    MarkerCanvas.UpdateLayout();
                                })
                                .DisposeWith(_selectionDisposable);
                        });

                    // Reset front state
                    codeViewModel.SelectedTextualSourceObject = null;
                    codeViewModel.DetailViewModel = null;

                    // Bind navigation location
                    codeViewModel.WhenAnyValue(y => y.NavigationLocation)
                        .WhereNotNull()
                        .Subscribe(location => UpdateNavigationLocation(codeViewModel, location!));
                });
        }

        /// <summary>
        /// Editor template bindings
        /// </summary>
        private void OnEditorTemplateApplied(object? sender, TemplateAppliedEventArgs e)
        {
            // Bind scrolling
            if (Editor
                    .GetVisualDescendants()
                    .OfType<ScrollViewer>()
                    .FirstOrDefault() is { } scroller)
            {
                scroller.ScrollChanged += OnScrollChanged;
            }
        }

        /// <summary>
        /// On scroll changes
        /// </summary>
        private void OnScrollChanged(object? sender, ScrollChangedEventArgs e)
        {
            if (_selectedCodeFileViewModel == null)
            {
                return;
            }
            
            // Get navigation state
            if (!_navigationStates.TryGetValue(_selectedCodeFileViewModel, out var navigationState))
            {
                return;
            }

            // First visual line
            if (Editor.TextArea.TextView.VisualLines.FirstOrDefault() is not { } line)
            {
                return;
            }

            navigationState.Line = line.FirstDocumentLine.LineNumber;
            navigationState.Column = 0;
        }

        /// <summary>
        /// Invoked on shader additions
        /// </summary>
        private void OnShaderAdded(CodeContentViewModel codeViewModel, ShaderViewModel shaderViewModel)
        {
            CompositeDisposable disposables = new();
            
            // Bind objects
            shaderViewModel.ValidationObjects.ToObservableChangeSet()
                .AsObservableList()
                .Connect()
                .OnItemAdded(x => OnValidationObjectAdded(codeViewModel, x))
                .OnItemRemoved(x => OnValidationObjectRemoved(codeViewModel, x))
                .Subscribe()
                .DisposeWith(disposables);

            // Raise navigation changes on file additions
            shaderViewModel.FileViewModels.ToObservableChangeSet()
                .AsObservableList()
                .Connect()
                .OnItemAdded(x =>
                {
                    if (codeViewModel.NavigationLocation != null)
                    {
                        UpdateNavigationLocation(codeViewModel, codeViewModel.NavigationLocation);
                    }
                })
                .Subscribe()
                .DisposeWith(disposables);
                        
            // Bind status
            shaderViewModel.WhenAnyValue(o => o.AsyncStatus).Subscribe(status =>
            {
                if (status.HasFlag(AsyncObjectStatus.NotFound))
                {
                    Editor.Text = Studio.Resources.Resources.Shader_NotFound;
                }
                else if (status.HasFlag(AsyncObjectStatus.NoDebugSymbols))
                {
                    Editor.Text = Studio.Resources.Resources.Shader_NoDebugSymbols;
                }
            }).DisposeWith(disposables);
            
            _shaderDisposables.Add(shaderViewModel, disposables);
        }

        /// <summary>
        /// Invoked on shader removal
        /// </summary>
        private void OnShaderRemoved(ShaderViewModel shaderViewModel)
        {
            if (_shaderDisposables.TryGetValue(shaderViewModel, out CompositeDisposable? disposables))
            {
                disposables.Clear();
                _shaderDisposables.Remove(shaderViewModel);
            }
        }

        /// <summary>
        /// Invoked on detail requests
        /// </summary>
        private void OnDetailCommand(ITextualSourceObject sourceObject)
        {
            // Validation
            if (DataContext is not CodeContentViewModel
                {
                    PropertyCollection: {} property
                } vm)
            {
                return;
            }

            InstrumentationVersion? version = null;

            // If validation, bind instrumentation
            if (sourceObject is ValidationObject { ShaderViewModel: not null } validationObject )
            {
                // Check if there's any detailed info at all
                if (vm.DetailViewModel == null && !ShaderDetailUtils.CanDetailCollect(validationObject, validationObject.ShaderViewModel))
                {
                    vm.DetailViewModel = new NoDetailViewModel
                    {
                        Object = validationObject.ShaderViewModel,
                        PropertyCollection = vm.PropertyCollection
                    };
                    return;
                }
            
                // Ensure detailed collection has started
                version = ShaderDetailUtils.BeginDetailedCollection(validationObject.ShaderViewModel, property);
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
                    // Multi-views do not have a specific shader
                    Object =  GetSingleViewShader(vm),
                    PropertyCollection = vm.PropertyCollection,
                    Version = version
                };
            }).DisposeWithClear(_detailDisposable);
        }

        private void UpdateNavigationLocation(CodeContentViewModel codeViewModel, NavigationLocation location)
        {
            // Multi-view ignored
            if (GetSingleViewShader(codeViewModel) is not {} shaderViewModel)
            {
                return;
            }
            
            // Attempt to find file vm
            ShaderFileViewModel? fileViewModel = shaderViewModel.FileViewModels.FirstOrDefault(x => x.UID == location.Location.FileUID);
            if (fileViewModel == null)
            {
                return;
            }

            // Update selected file
            codeViewModel.SelectedFileViewModel = fileViewModel;
            codeViewModel.SelectedTextualSourceObject = location.Object;

            // Scroll to target
            // TODO: 10 is a total guess, we need to derive it from the height, but that doesn't exist yet.
            //       Probably a delayed action?
            Editor.TextArea.Caret.Line = location.Location.Line - 10;
            Editor.TextArea.Caret.Column = 0;
            Editor.TextArea.Caret.BringCaretToView();
                    
            // Invalidate marker layout
            MarkerCanvas.UpdateLayout();
        }

        /// <summary>
        /// Get the single shader view model
        /// </summary>
        private ShaderViewModel? GetSingleViewShader(CodeContentViewModel codeViewModel)
        {
            return codeViewModel.Content as  ShaderViewModel;
        }
        
        /// <summary>
        /// Invoked on object added
        /// </summary>
        /// <param name="validationObject"></param>
        private void OnValidationObjectAdded(CodeContentViewModel viewModel, ValidationObject validationObject)
        {
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
        private void OnValidationObjectRemoved(CodeContentViewModel viewModel, ValidationObject validationObject)
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
        /// Disposable for detailed data
        /// </summary>
        private CompositeDisposable _detailDisposable = new();

        /// <summary>
        /// Disposable for single-file content reacts
        /// </summary>
        private CompositeDisposable _selectionDisposable = new();

        /// <summary>
        /// Disposables for shader states
        /// </summary>
        private Dictionary<ShaderViewModel, CompositeDisposable> _shaderDisposables = new();

        private class NavigationState
        {
            public int Line;
            public int Column;
        }
        
        /// <summary>
        /// Shared weak navigation states
        /// </summary>
        private static ConditionalWeakTable<CodeFileViewModel, NavigationState> _navigationStates = new();

        /// <summary>
        /// Current file model
        /// </summary>
        private CodeFileViewModel? _selectedCodeFileViewModel;
    }
}