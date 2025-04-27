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
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Objects;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Services;

namespace Studio.App.Commands.Cli;

public class CliWorkspaceExtension : IWorkspaceExtension
{
    /// <summary>
    /// Install this extension
    /// </summary>
    public void Install(IWorkspaceViewModel workspaceViewModel)
    {
        _shaderCollection  = workspaceViewModel.PropertyCollection.GetProperty<IShaderCollectionViewModel>();
        _shaderCodeService = workspaceViewModel.PropertyCollection.GetService<IShaderCodeService>();

        // Subscribe to all validation objects
        if (workspaceViewModel.PropertyCollection.GetProperty<MessageCollectionViewModel>() is { } messageCollection)
        {
            messageCollection.ValidationObjects.ToObservableChangeSet()
                .AsObservableList()
                .Connect()
                .OnItemAdded(OnValidationObjectAdded)
                .Subscribe();
        }
    }

    /// <summary>
    /// Invoked on validation object additions
    /// </summary>
    private void OnValidationObjectAdded(ValidationObject obj)
    {
        obj.WhenAnyValue(x => x.Segment).WhereNotNull().Subscribe(segment =>
        {
            // Whenever a validation object is enqueued, immediately start pooling its shader contents
            // for later reporting purposes.
            if (_shaderCollection?.GetOrAddShader(segment.Location.SGUID) is { } shader)
            {
                _shaderCodeService?.EnqueueShaderContents(shader);
                _shaderCodeService?.EnqueueShaderIL(shader);
                
                // Release the shader once the contents have been enqueued
                _shaderCodeService?.EnqueueReleaseShader(shader);
            }
            else
            {
                Logging.Error($"Failed to enqueue shader contents for SGUID {segment.Location.SGUID}");
            }
        });
    }

    /// <summary>
    /// Workspace shaders
    /// </summary>
    private IShaderCollectionViewModel? _shaderCollection;
    
    /// <summary>
    /// Workspace code service
    /// </summary>
    private IShaderCodeService? _shaderCodeService;
}