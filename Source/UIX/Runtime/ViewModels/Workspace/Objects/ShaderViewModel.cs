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
        /// Contents of this shader
        /// </summary>
        public string IL
        {
            get => _il;
            set => this.RaiseAndSetIfChanged(ref _il, value);
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
        /// Internal contents
        /// </summary>
        private string _il = string.Empty;

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