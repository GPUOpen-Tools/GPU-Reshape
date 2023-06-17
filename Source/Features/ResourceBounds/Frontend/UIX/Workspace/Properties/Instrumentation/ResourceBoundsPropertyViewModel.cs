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

using System.Windows.Input;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Models.Workspace;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace GRS.Features.ResourceBounds.UIX.Workspace.Properties.Instrumentation
{
    public class ResourceBoundsPropertyViewModel : BasePropertyViewModel, IInstrumentationProperty, IClosableObject
    {
        /// <summary>
        /// Resource bounds feature info
        /// </summary>
        public FeatureInfo FeatureInfo { get; set; }
        
        /// <summary>
        /// Invoked on close requests
        /// </summary>
        public ICommand? CloseCommand { get; set; }
        
        /// <summary>
        /// Constructor
        /// </summary>
        public ResourceBoundsPropertyViewModel() : base("Resource Bounds", PropertyVisibility.WorkspaceTool)
        {
            CloseCommand = ReactiveCommand.Create(OnClose);
        }

        /// <summary>
        /// Invoked on close
        /// </summary>
        private void OnClose()
        {
            this.DetachFromParent();
        }

        /// <summary>
        /// Commit all state
        /// </summary>
        /// <param name="state"></param>
        public void Commit(InstrumentationState state)
        {
            state.FeatureBitMask |= FeatureInfo.FeatureBit;
        }
    }
}