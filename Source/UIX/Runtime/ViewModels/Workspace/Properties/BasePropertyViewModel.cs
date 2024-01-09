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

using DynamicData;
using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Workspace.Properties
{
    public abstract class BasePropertyViewModel : ReactiveObject, IPropertyViewModel
    {
        /// <summary>
        /// Name of this property
        /// </summary>
        public string Name
        {
            get => _name;
            set => this.RaiseAndSetIfChanged(ref _name, value);
        }

        /// <summary>
        /// Visibility of this property
        /// </summary>
        public PropertyVisibility Visibility => _visibility;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="name">Given initial name</param>
        /// <param name="visibility">Final visibility</param>
        public BasePropertyViewModel(string name, PropertyVisibility visibility)
        {
            Name = name;
            _visibility = visibility;
        }

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
            set => this.RaiseAndSetIfChanged(ref _connectionViewModel, value);
        }
        
        /// <summary>
        /// Internal connection
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Internal name
        /// </summary>
        private string _name = string.Empty;

        /// <summary>
        /// Internal visibility
        /// </summary>
        private PropertyVisibility _visibility;

        /// <summary>
        /// Invoked on destruction (kept in for overrides)
        /// </summary>
        public virtual void Destruct()
        {
            // Poof
        }
    }
}