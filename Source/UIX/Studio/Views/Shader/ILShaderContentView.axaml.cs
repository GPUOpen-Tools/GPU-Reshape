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
            var registryOptions = new RegistryOptions(ThemeName.DarkPlus);

            // Create a text mate instance
            var textMate = Editor.InstallTextMate(registryOptions);

            // Set the default grammar (just assume .hlsl)
            textMate.SetGrammar(registryOptions.GetScopeByLanguageId(registryOptions.GetLanguageByExtension(".cpp").Id));

            // Common styling
            Editor.Options.IndentationSize = 4;
            
            // Bind contents
            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Cast<ILShaderContentViewModel>()
                .WhereNotNull()
                .Subscribe(x =>
            {
                x.Object.WhenAnyValue(x => x.IL).WhereNotNull().Subscribe(il =>
                {
                    Editor.Text = il;
                });
            });
        }
    }
}