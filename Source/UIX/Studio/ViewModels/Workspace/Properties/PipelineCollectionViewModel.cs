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
using System.Collections.Generic;
using System.Collections.ObjectModel;
using DynamicData;
using ReactiveUI;

namespace Studio.ViewModels.Workspace.Properties
{
    public class PipelineCollectionViewModel : ReactiveObject, IPipelineCollectionViewModel
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name { get; } = "Pipelines";

        /// <summary>
        /// Visibility of this property
        /// </summary>
        public PropertyVisibility Visibility => PropertyVisibility.WorkspaceTool;

        /// <summary>
        /// Parent property
        /// </summary>
        public IPropertyViewModel? Parent { get; set; }

        /// <summary>
        /// Child properties
        /// </summary>
        public ISourceList<IPropertyViewModel> Properties { get; set; } = new SourceList<IPropertyViewModel>();

        /// <summary>
        /// All services
        /// </summary>
        public ISourceList<IPropertyService> Services { get; set; } = new SourceList<IPropertyService>();

        /// <summary>
        /// All pipelines within this collection
        /// </summary>
        public ObservableCollection<Objects.PipelineViewModel> Pipelines { get; } = new();
        
        /// <summary>
        /// View model associated with this property
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set
            {
                this.RaiseAndSetIfChanged(ref _connectionViewModel, value);

                OnConnectionChanged();
            }
        }

        /// <summary>
        /// Add a new pipeline to this collection
        /// </summary>
        /// <param name="pipelineViewModel"></param>
        public void AddPipeline(Objects.PipelineViewModel pipelineViewModel)
        {
            // Flat view
            Pipelines.Add(pipelineViewModel);
            
            // Create lookup
            _pipelineModelGUID.Add(pipelineViewModel.GUID, pipelineViewModel);
        }

        /// <summary>
        /// Get a pipeline from this collection
        /// </summary>
        /// <param name="GUID"></param>
        /// <returns>null if not found</returns>
        public Objects.PipelineViewModel? GetPipeline(UInt64 GUID)
        {
            _pipelineModelGUID.TryGetValue(GUID, out Objects.PipelineViewModel? pipelineViewModel);
            return pipelineViewModel;
        }

        /// <summary>
        /// Get a pipeline from this collection, add if not found
        /// </summary>
        /// <param name="GUID"></param>
        /// <returns></returns>
        public Objects.PipelineViewModel GetOrAddPipeline(UInt64 GUID)
        {
            Objects.PipelineViewModel? pipelineViewModel = GetPipeline(GUID);
            
            // Create if not found
            if (pipelineViewModel == null)
            {
                pipelineViewModel = new Objects.PipelineViewModel()
                {
                    GUID = GUID
                };
                
                // Append
                AddPipeline(pipelineViewModel);
            }

            // OK
            return pipelineViewModel;
        }
        
        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            // Set connection
            _pipelinePoolingService.ConnectionViewModel = ConnectionViewModel;
            
            // Make visible
            Parent?.Services.Add(_pipelinePoolingService);
            
            // Register internal listeners
            _connectionViewModel?.Bridge?.Register(_pipelinePoolingService);

            CreateProperties();
        }

        /// <summary>
        /// Create all properties
        /// </summary>
        private void CreateProperties()
        {
            Properties.Clear();
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Deregister listeners
            _connectionViewModel?.Bridge?.Deregister(_pipelinePoolingService);
        }

        /// <summary>
        /// GUID to pipeline mappings
        /// </summary>
        private Dictionary<UInt64, Objects.PipelineViewModel> _pipelineModelGUID = new();

        /// <summary>
        /// Pooling listener
        /// </summary>
        private Services.PipelinePoolingService _pipelinePoolingService = new();

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}