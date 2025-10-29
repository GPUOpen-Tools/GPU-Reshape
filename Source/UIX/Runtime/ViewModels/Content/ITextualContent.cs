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

using Runtime.ViewModels.Shader;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Shader
{
    public interface ITextualContent : IShaderContentViewModel
    {
        /// <summary>
        /// The current selected object
        /// </summary>
        public ITextualSourceObject? SelectedTextualSourceObject { get; set; }
        
        /// <summary>
        /// Canvas view model
        /// </summary>
        public SourceObjectMarkerCanvasViewModel MarkerCanvasViewModel { get; }
        
        /// <summary>
        /// Is the overlay visible?
        /// </summary>
        public bool IsOverlayVisible();
        
        /// <summary>
        /// Is a location visible?
        /// </summary>
        public bool IsLocationVisible(ShaderLocation sourceObject);

        /// <summary>
        /// Transform a shader location
        /// </summary>
        public int TransformLine(ShaderLocation shaderLocation);

        /// <summary>
        /// Transform a shader instruction
        /// </summary>
        public ShaderMultiAssociationViewModel<ShaderInstructionSourceAssociationViewModel>? TransformInstructionLine(AssembledInstructionMapping mapping);

        /// <summary>
        /// Transform a shader line
        /// </summary>
        public ShaderMultiAssociationViewModel<ShaderInstructionAssociationViewModel>? TransformSourceLine(int line);
    }
}