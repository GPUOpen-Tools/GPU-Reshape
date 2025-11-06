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

using System.Collections.Generic;
using System.Linq;
using DynamicData.Kernel;
using Runtime.ViewModels.Shader;
using Studio.Utils;
using Studio.ViewModels.Workspace.Objects;

namespace Runtime.Utils.Workspace
{
    public static class ShaderPathUtils
    {
        /// <summary>
        /// Find the best matching file view model from a path
        /// </summary>
        public static ShaderFileViewModel? FindBestMatchContainedFile(ShaderViewModel shaderViewModel, string path)
        {
            // TODO: We definitely need a lookup for this
            
            // Base set of mappings
            var files = shaderViewModel.FileViewModels
                .Select(x => KeyValuePair.Create(PathUtils.StandardizePartialPath(x.Filename), x))
                .ToArray();

            // Keep going until we find a match
            while (files.Length > 0)
            {
                string searchPath = PathUtils.StandardizePartialPath(path);
                while (!string.IsNullOrEmpty(searchPath))
                {
                    // Has candidate?
                    var file = files.FirstOrOptional(x => x.Key == searchPath);
                    if (file.HasValue)
                    {
                        return file.Value.Value;
                    }
            
                    // None matching, go up on the search path
                    searchPath = PathUtils.RemoveLeadingDirectory(searchPath);
                }

                // Nothing found for the current leading file directory, remove it
                files = files
                    .Where(x => x.Key.Contains('/'))
                    .Select(x => KeyValuePair.Create(PathUtils.RemoveLeadingDirectory(x.Key), x.Value))
                    .ToArray();
            }
            

            // Just try the base one
            return null;
        }
    }
}