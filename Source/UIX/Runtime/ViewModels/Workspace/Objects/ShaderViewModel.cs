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

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using ReactiveUI;
using Runtime.ViewModels.Shader;
using Studio.Models.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Objects
{
    public class ShaderViewModel : ReactiveObject
    {
        /// <summary>
        /// Shader GUID
        /// </summary>
        public UInt64 GUID { get; set; }

        /// <summary>
        /// Contents of this shader
        /// </summary>
        public string Filename
        {
            get => _filename;
            set => this.RaiseAndSetIfChanged(ref _filename, value);
        }

        /// <summary>
        /// Contents of this shader
        /// </summary>
        public string Contents
        {
            get => _contents;
            set => this.RaiseAndSetIfChanged(ref _contents, value);
        }

        /// <summary>
        /// Program of this shader
        /// </summary>
        public Models.IL.Program? Program
        {
            get => _program;
            set => this.RaiseAndSetIfChanged(ref _program, value);
        }

        /// <summary>
        /// Contents of this shader
        /// </summary>
        public string BlockGraph
        {
            get => _blockGraph;
            set => this.RaiseAndSetIfChanged(ref _blockGraph, value);
        }

        /// <summary>
        /// Current asynchronous status
        /// </summary>
        public AsyncShaderStatus AsyncStatus
        {
            get => _asyncStatus;
            set => this.RaiseAndSetIfChanged(ref _asyncStatus, value);
        }
        
        /// <summary>
        /// All condensed messages
        /// </summary>
        public ObservableCollection<ValidationObject> ValidationObjects { get; } = new();

        /// <summary>
        /// File view models
        /// </summary>
        public ObservableCollection<ShaderFileViewModel> FileViewModels { get; } = new();

        /// <summary>
        /// Add a new validation object to this shader
        /// </summary>
        /// <param name="key"></param>
        /// <param name="validationObject"></param>
        public void AddValidationObject(uint key, ValidationObject validationObject)
        {
            _reducedValidationObjects.Add(key, validationObject);
        }

        /// <summary>
        /// Get a validation object from this shader
        /// </summary>
        /// <param name="key"></param>
        /// <returns>null if not found</returns>
        public ValidationObject? GetValidationObject(uint key)
        {
            _reducedValidationObjects.TryGetValue(key, out ValidationObject? validationObject);
            return validationObject;
        }

        /// <summary>
        /// Internal contents
        /// </summary>
        private string _contents = string.Empty;

        /// <summary>
        /// Internal program
        /// </summary>
        private Models.IL.Program? _program;

        /// <summary>
        /// Internal filename
        /// </summary>
        private string _filename = string.Empty;

        /// <summary>
        /// Internal filename
        /// </summary>
        private string _blockGraph = string.Empty;
        
        /// <summary>
        /// All reduced resource messages
        /// </summary>
        private Dictionary<uint, ValidationObject> _reducedValidationObjects = new();

        /// <summary>
        /// Internal asynchronous status
        /// </summary>
        private AsyncShaderStatus _asyncStatus = AsyncShaderStatus.None;
    }
}