// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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

using System.Windows.Input;
using Avalonia.Media;
using Studio.ViewModels.Documents;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Shader
{
    public interface IShaderContentViewModel
    {
        /// <summary>
        /// Given creation descriptor
        /// </summary>
        public ShaderDescriptor? Descriptor { set; }
        
        /// <summary>
        /// Content icon
        /// </summary>
        public StreamGeometry? Icon { get; set; }
        
        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public IPropertyViewModel? PropertyCollection { get; set; }

        /// <summary>
        /// Selection command
        /// </summary>
        public ICommand? OnSelected { get; }
        
        /// <summary>
        /// Is this model active?
        /// </summary>
        public bool IsActive { get; set; }

        /// <summary>
        /// Underlying object
        /// </summary>
        public Workspace.Objects.ShaderViewModel? Object { get; set; }
    }
}
