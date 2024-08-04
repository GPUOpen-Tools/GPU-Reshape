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
using Avalonia.Controls;
using Avalonia.Controls.Templates;
using Avalonia.Metadata;
using Studio.ViewModels.Controls;

namespace Studio.Views.Controls
{
    public class PropertyGridDataTemplate : IDataTemplate
    {
        /// <summary>
        /// All templates
        /// </summary>
        [Content] 
        public Dictionary<Type, IDataTemplate> Templates { get; } = new();

        /// <summary>
        /// Build a template from given data
        /// Must be field
        /// </summary>
        public Control? Build(object? data)
        {
            if (data is not PropertyFieldViewModel field)
            {
                return null;
            }

            // Try to build from templates
            return Templates.TryGetValue(field.Value.GetType(), out IDataTemplate? template) ? template.Build(data) : null;
        }

        /// <summary>
        /// Match given data
        /// Must be field
        /// </summary>
        public bool Match(object? data)
        {
            return data is PropertyFieldViewModel;
        }
    }
}