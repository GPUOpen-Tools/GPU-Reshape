using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using AvaloniaEdit.TextMate;
using AvaloniaEdit.Utils;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.ViewModels.Workspace.Objects;
using Studio.Views.Editor;
using TextMateSharp.Grammars;
using ShaderViewModel = Studio.ViewModels.Documents.ShaderViewModel;

namespace Studio.Views.Documents
{
    public partial class ShaderView : UserControl
    {
        public ShaderView()
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
            this.WhenAnyValue(x => x.DataContext).Subscribe(x =>
            {
                if (x is ShaderViewModel vm)
                {
                    vm.Object.WhenAnyValue(x => x.Contents).WhereNotNull().Subscribe(contents =>
                    {
                        Editor.Text = contents;

                        // Set objects
                        _validationBackgroundRenderer.ValidationObjects = vm.Object.ValidationObjects;

                        // Bind objects
                        vm.Object.ValidationObjects.ToObservableChangeSet()
                            .AsObservableList()
                            .Connect()
                            .OnItemAdded(OnValidationObjectAdded)
                            .OnItemRemoved(OnValidationObjectAdded)
                            .Subscribe();
                    });
                }
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