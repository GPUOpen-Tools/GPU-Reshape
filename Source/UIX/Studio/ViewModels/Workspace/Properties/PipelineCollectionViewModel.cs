// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Fatalist Development AB
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
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            CreateProperties();
        }

        /// <summary>
        /// Create all properties
        /// </summary>
        private void CreateProperties()
        {
            Properties.Clear();

            if (_connectionViewModel == null)
            {
                return;
            }
        }

        /// <summary>
        /// Internal view model
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;
    }
}