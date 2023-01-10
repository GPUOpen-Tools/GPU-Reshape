using System;
using System.Reactive.Linq;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using AvaloniaEdit.TextMate;
using AvaloniaEdit.Utils;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.Extensions;
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
                .Subscribe(x =>
                {
                    // Update services
                    _validationTextMarkerService.ShaderContentViewModel = x;
                    _validationBackgroundRenderer.ShaderContentViewModel = x;
                    
                    // Bind object model
                    x.WhenAnyValue(y => y.Object).WhereNotNull().Subscribe(_object =>
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
                    });
                    
                    // Bind selected contents
                    x.WhenAnyValue(y => y.SelectedShaderFileViewModel, y => y?.Contents)
                        .WhereNotNull()
                        .Subscribe(contents =>
                        {
                            // Clear and set, avoids internal replacement reformatting hell
                            Editor.Text = string.Empty;
                            Editor.Text = contents;
                            
                            _validationTextMarkerService.ResumarizeValidationObjects();
                        });
                });
        }

        /// <summary>
        /// Invoked on object added
        /// </summary>
        /// <param name="validationObject"></param>
        private void OnValidationObjectAdded(ValidationObject validationObject)
        {
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
    }
}