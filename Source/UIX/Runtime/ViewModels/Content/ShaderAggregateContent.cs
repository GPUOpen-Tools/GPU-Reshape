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
using System.Collections.ObjectModel;
using DynamicData;
using DynamicData.Binding;
using Studio.ViewModels.Code;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Services;

namespace Studio.ViewModels.Shader
{
    public static class ShaderAggregateContentExtensions
    {
        /// <summary>
        /// Get a set of observable shaders from a content view model
        /// </summary>
        public static ObservableCollection<ShaderViewModel>? GetObservableShaders(this IContentViewModel self)
        {
            switch (self.Content)
            {
                default:
                {
                    return null;
                }
                case ShaderViewModel shaderViewModel:
                {
                    // Statically known
                    return [shaderViewModel];
                }
                case CodeFileViewModel codeFileViewModel:
                {
                    // Get the associations from the file
                    if (self.PropertyCollection?.GetService<IFileCodeService>()?.GetOrAddMappingSet(codeFileViewModel) is not { } mappingSet)
                    {
                        return null;
                    }

                    ObservableCollection<ShaderViewModel> collection = new();

                    // Mirror the collection
                    mappingSet.Mappings.ToObservableChangeSet()
                        .OnItemAdded(x => collection.Add(x.ShaderViewModel))
                        .OnItemRemoved(x => collection.Remove(x.ShaderViewModel))
                        .Subscribe();

                    return collection;
                }
            }
        }
    }
}
