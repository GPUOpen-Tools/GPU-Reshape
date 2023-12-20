// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
using Message.CLR;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Instrumentation;

namespace Runtime.ViewModels.Objects
{
    public class PipelineIdentifierViewModel : ReactiveObject, IInstrumentableObject
    {
        /// <summary>
        /// Underlying model
        /// </summary>
        public PipelineIdentifier Model;

        /// <summary>
        /// Owning workspace
        /// </summary>
        public IWorkspaceViewModel? Workspace;
        
        /// <summary>
        /// Non reactive, identifiers are pooled in a pseudo-virtualized manner
        /// </summary>
        public bool HasBeenPooled { get; set; }

        /// <summary>
        /// Given pipeline stage
        /// </summary>
        public PipelineType Stage
        {
            get => _stage;
            set => this.RaiseAndSetIfChanged(ref _stage, value);
        }

        /// <summary>
        /// Readable descriptor
        /// </summary>
        public string Descriptor
        {
            get => Model.Descriptor;
            set
            {
                if (value != Model.Descriptor)
                {
                    Model.Descriptor = value;
                    this.RaisePropertyChanged(nameof(Descriptor));
                }
            }
        }

        /// <summary>
        /// Get the targetable instrumentation property
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel? GetOrCreateInstrumentationProperty()
        {
            // Get collection
            var pipelineCollectionViewModel = Workspace?.PropertyCollection.GetProperty<IPipelineCollectionViewModel>();
            if (pipelineCollectionViewModel == null)
            {
                return null;
            }

            // Find or create property
            var pipelineViewModel = pipelineCollectionViewModel.GetPropertyWhere<PipelineViewModel>(x => x.Pipeline.GUID == Model.GUID);
            if (pipelineViewModel == null)
            {
                pipelineCollectionViewModel.Properties.Add(pipelineViewModel = new PipelineViewModel()
                {
                    Parent = pipelineCollectionViewModel,
                    ConnectionViewModel = pipelineCollectionViewModel.ConnectionViewModel,
                    Pipeline = Model
                });
            }
            
            // OK
            return pipelineViewModel;
        }

        /// <summary>
        /// Instrumentation handler
        /// </summary>
        public InstrumentationState InstrumentationState
        {
            get
            {
                return Workspace?.PropertyCollection
                    .GetProperty<IPipelineCollectionViewModel>()?
                    .GetPropertyWhere<PipelineViewModel>(x => x.Pipeline.GUID == Model.GUID)?
                    .InstrumentationState ?? new InstrumentationState();   
            }
        }

        /// <summary>
        /// Global UID
        /// </summary>
        public UInt64 GUID
        {
            get => Model.GUID;
            set
            {
                if (value != Model.GUID)
                {
                    Model.GUID = value;
                    this.RaisePropertyChanged(nameof(GUID));
                }
            }
        }

        /// <summary>
        /// Get the workspace
        /// </summary>
        /// <returns></returns>
        public IPropertyViewModel? GetWorkspace()
        {
            return Workspace?.PropertyCollection;
        }

        /// <summary>
        /// Internal stage
        /// </summary>
        private PipelineType _stage;
    }
}