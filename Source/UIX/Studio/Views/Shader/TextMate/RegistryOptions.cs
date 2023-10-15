// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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