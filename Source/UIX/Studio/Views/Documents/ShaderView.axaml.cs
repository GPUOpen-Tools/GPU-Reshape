using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using AvaloniaEdit.TextMate;
using ReactiveUI;
using Studio.ViewModels.Documents;
using TextMateSharp.Grammars;

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
            
            // Bind contents
            this.WhenAnyValue(x => x.DataContext).Subscribe(x =>
            {
                if (x is ShaderViewModel vm)
                {
                    vm.WhenAnyValue(x => x.Object).Subscribe(o =>
                    {
                        Editor.Text = o.Contents;
                    });
                }
            });

        }
    }
}