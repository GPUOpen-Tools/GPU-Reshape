using System;
using System.Collections.Generic;
using System.IO;
using Avalonia;
using Avalonia.Platform;
using TextMateSharp.Internal.Grammars.Reader;
using TextMateSharp.Internal.Themes.Reader;
using TextMateSharp.Internal.Types;
using TextMateSharp.Registry;
using TextMateSharp.Themes;

namespace Studio.Views.Shader.TextMate
{
    public class RegistryOptions : IRegistryOptions
    {
        /// <summary>
        /// Construct theme from a scope
        /// </summary>
        public IRawTheme GetTheme(string scopeName)
        {
            // Try to fetch resource
            Stream? stream = AvaloniaLocator.Current.GetService<IAssetLoader>()?.Open(new Uri($"avares://GPUReshape/Resources/TextMate/Themes/{scopeName}.json"));
            if (stream == null)
            {
                return null!;
            }

            using StreamReader reader = new StreamReader(stream);
            return ThemeReader.ReadThemeSync(reader);
        }

        /// <summary>
        /// Construct grammar from a scope
        /// </summary>
        public IRawGrammar GetGrammar(string scopeName)
        {
            // Try to fetch resource
            Stream? stream = AvaloniaLocator.Current.GetService<IAssetLoader>()?.Open(new Uri($"avares://GPUReshape/Resources/TextMate/Grammars/{scopeName}.json"));
            if (stream == null)
            {
                return null!;
            }

            using StreamReader reader = new StreamReader(stream);
            return GrammarReader.ReadGrammarSync(reader);
        }

        /// <summary>
        /// Get all injections from a scope
        /// </summary>
        public ICollection<string> GetInjections(string scopeName)
        {
            return null!;
        }

        /// <summary>
        /// Get the default theme
        /// </summary>
        public IRawTheme GetDefaultTheme()
        {
            return GetTheme("DarkIL");
        }
    }
}