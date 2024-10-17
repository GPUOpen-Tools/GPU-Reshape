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
using ReactiveUI;
using Runtime.Resources;
using Runtime.ViewModels.Workspace.Properties.Bus;
using Studio.Models.Instrumentation;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace.Objects
{
    public class MissingDetailViewModel : ReactiveObject, IValidationDetailViewModel
    {
        /// <summary>
        /// Workspace within this overview
        /// </summary>
        public IPropertyViewModel? PropertyCollection
        {
            get => _propertyCollection;
            set => this.RaiseAndSetIfChanged(ref _propertyCollection, value);
        }

        /// <summary>
        /// Underlying object
        /// </summary>
        public ShaderViewModel? Object
        {
            get => _object;
            set => this.RaiseAndSetIfChanged(ref _object, value);
        }

        /// <summary>
        /// Version this view model is waiting for
        /// </summary>
        public InstrumentationVersion? Version
        {
            get => _version;
            set => this.RaiseAndSetIfChanged(ref _version, value);
        }
        
        /// <summary>
        /// Current status message
        /// </summary>
        public string StatusMessage
        {
            get => _statusMessage;
            set => this.RaiseAndSetIfChanged(ref _statusMessage, value);
        }

        /// <summary>
        /// Current status info, may be empty
        /// </summary>
        public string StatusInfo
        {
            get => _statusInfo;
            set => this.RaiseAndSetIfChanged(ref _statusInfo, value);
        }

        /// <summary>
        /// Any status info to show?
        /// </summary>
        public bool HasStatus
        {
            get => _hasStatus;
            set => this.RaiseAndSetIfChanged(ref _hasStatus, value);
        }

        /// <summary>
        /// Constructor
        /// </summary>
        public MissingDetailViewModel()
        {
            StatusMessage = Resources.Detail_Waiting_Compilation;

            // Bind to version changes
            this.WhenAnyValue(x => x.Version).Subscribe(_ => OnVersionChange());
        }

        /// <summary>
        /// Invoked on version changes
        /// </summary>
        private void OnVersionChange()
        {
            if (_version == null)
            {
                return;
            }
            
            // Bind to committed version changes
            _propertyCollection?
                .GetInstrumentationVersionController()
                .WhenAnyValue(x => x.CommittedVersionID)
                .Subscribe(OnCommitVersionChange);
        }

        /// <summary>
        /// Invoked on commited version changes
        /// </summary>
        /// <param name="commit"></param>
        private void OnCommitVersionChange(InstrumentationVersion commit)
        {
            // Has the version completed?
            if (_version == null || !InstrumentationVersion.Completed(_version.Value, commit))
            {
                return;
            }

            // Update status
            StatusMessage = Resources.Detail_Waiting_Streaming;
            StatusInfo = Resources.Detail_Waiting_Streaming_Info;
            HasStatus = true;
        }

        /// <summary>
        /// Internal object
        /// </summary>
        private ShaderViewModel? _object;

        /// <summary>
        /// Underlying view model
        /// </summary>
        private IPropertyViewModel? _propertyCollection;

        /// <summary>
        /// Internal status message
        /// </summary>
        private string _statusMessage;

        /// <summary>
        /// Internal status info
        /// </summary>
        private string _statusInfo;

        /// <summary>
        /// Internal status state
        /// </summary>
        private bool _hasStatus = false;
        
        /// <summary>
        /// Internal version
        /// </summary>
        private InstrumentationVersion? _version;
    }
}