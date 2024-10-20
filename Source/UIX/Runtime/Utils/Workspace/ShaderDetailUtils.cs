﻿// 
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

using DynamicData;
using Runtime.Models.Objects;
using Runtime.ViewModels.Workspace.Properties.Bus;
using Studio.Models.Instrumentation;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Config;

namespace Runtime.Utils.Workspace
{
    public static class ShaderDetailUtils
    {
        /// <summary>
        /// Check if a validation object can produce detailed information
        /// </summary>
        /// <param name="validationObject">object to check</param>
        /// <param name="shaderViewModel">shader to check against, object must belong to it</param>
        /// <returns>true if can collect</returns>
        public static bool CanDetailCollect(ValidationObject validationObject, ShaderViewModel shaderViewModel)
        {
            // If the program is not ready, cannot assume no
            if (shaderViewModel.Program is not { } program)
            {
                return true;
            }
            
            // If either no traits of segment yet, cannot assume no
            if (validationObject is not { Traits: { } traits, Segment: { } segment })
            {
                return true;
            }

            // Check with traits
            return traits.ProducesDetailDataFor(program, segment.Location);
        }
        
        /// <summary>
        /// Begin detailed collection of shader data
        /// </summary>
        public static InstrumentationVersion BeginDetailedCollection(ShaderViewModel _object, IPropertyViewModel propertyCollection)
        {
            // Get collection
            var shaderCollectionViewModel = propertyCollection.GetProperty<IShaderCollectionViewModel>();
            if (shaderCollectionViewModel == null)
            {
                return InstrumentationBusVersionController.NoVersionID;
            }

            // Find or create property
            var shaderViewModel = shaderCollectionViewModel.GetPropertyWhere<Studio.ViewModels.Workspace.Properties.Instrumentation.ShaderViewModel>(x => x.Shader.GUID == _object.GUID);
            if (shaderViewModel == null)
            {
                shaderCollectionViewModel.Properties.Add(shaderViewModel = new Studio.ViewModels.Workspace.Properties.Instrumentation.ShaderViewModel()
                {
                    Parent = shaderCollectionViewModel,
                    ConnectionViewModel = shaderCollectionViewModel.ConnectionViewModel,
                    Shader = new ShaderIdentifier()
                    {
                        GUID = _object.GUID,
                        Descriptor = $"Shader {_object.GUID} - {System.IO.Path.GetFileName(_object.Filename)}"
                    }
                });
            }

            // Enable detailed instrumentation on the shader
            if (shaderViewModel.GetProperty<InstrumentationConfigViewModel>() is { } instrumentationConfigViewModel && !instrumentationConfigViewModel.Detail)
            {
                InstrumentationVersion version = shaderViewModel.GetNextInstrumentationVersion();
                instrumentationConfigViewModel.Detail = true;
                return version;
            }

            return InstrumentationBusVersionController.NoVersionID;
        }
    }
}