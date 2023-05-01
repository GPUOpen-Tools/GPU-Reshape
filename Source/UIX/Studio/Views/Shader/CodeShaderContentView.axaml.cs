using System;
using System.Linq;
using System.Linq.Expressions;
using System.Reactive;
using System.Reactive.Linq;
using System.Reactive.Subjects;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using AvaloniaEdit.TextMate;
using AvaloniaEdit.Utils;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
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
                            .OnItemRemoved(OnValidationObjectAdded)
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

                            _validationTextMarkerService.ResumarizeValidationObjects();
                        });

                    // Bind navigation location
                    codeViewModel.WhenAnyValue(y => y.NavigationLocation)
                        .WhereNotNull()
                        .Subscribe(location => UpdateNavigationLocation(codeViewModel, location!.Value));
                });

            // Bind object change
            ValidationObjectChanged
                .Window(() => Observable.Timer(TimeSpan.FromMilliseconds(250)))
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(_ => OnValidationObjectChanged());
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
            Editor.TextArea.Caret.Line = location.Line;
            Editor.TextArea.Caret.Column = location.Column;
            Editor.TextArea.Caret.BringCaretToView();
        }
        
        /// <summary>
        /// Invoked on object changes
        /// </summary>
        private void OnValidationObjectChanged()
        {
            Editor.TextArea.TextView.Redraw();
        }

        /// <summary>
        /// Invoked on object added
        /// </summary>
        /// <param name="validationObject"></param>
        private void OnValidationObjectAdded(ValidationObject validationObject)
        {
            // Bind count and contents
            validationObject
                .WhenAnyValue(x => x.Count, x => x.Content)
                .Subscribe(x => ValidationObjectChanged.OnNext(new Unit()));

            _validationTextMarkerService.Add(validationObject);
        }

        /// <summary>
        /// Invoked on object removed
        /// </summary>
        /// <param name="validationObject"></param>
        private void OnValidationObjectRemoved(ValidationObject validationObject)
        {
            _validationTextMarkerService.Remove(validationObject);
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
        /// Internal proxy observable
        /// </summary>
        private ISubject<Unit> ValidationObjectChanged = new Subject<Unit>();
    }
}