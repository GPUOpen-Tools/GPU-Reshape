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

using System;
using System.Windows.Input;
using DynamicData;
using ReactiveUI;
using Runtime.Models.Objects;
using Studio.Models.Workspace;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;
using Studio.ViewModels.Workspace.Properties.Config;

namespace GRS.Features.ResourceBounds.UIX.Workspace.Properties.Instrumentation
{
    public class LoopPropertyViewModel : BasePropertyViewModel, IInstrumentationProperty, IClosableObject
    {
        /// <summary>
        /// Loop feature
        /// </summary>
        public FeatureInfo FeatureInfo { get; set; }
        
        /// <summary>
        /// Invoked on close requests
        /// </summary>
        public ICommand? CloseCommand { get; set; }
        
        /// <summary>
        /// Constructor
        /// </summary>
        public LoopPropertyViewModel() : base("Loop", PropertyVisibility.WorkspaceTool)
        {
            CloseCommand = ReactiveCommand.Create(OnClose);

            // Bind connection
            this.WhenAnyValue(x => x.ConnectionViewModel).Subscribe(_ => CreateProperties());
        }

        /// <summary>
        /// Create all properties
        /// </summary>
        private void CreateProperties()
        {
            Properties.Clear();
                
            // Standard range
            Properties.AddRange(new IPropertyViewModel[]
            {
                new LoopInstrumentationConfigViewModel()
                {
                    Parent = this,
                    ConnectionViewModel = ConnectionViewModel
                }
            });
        }

        /// <summary>
        /// Invoked on close
        /// </summary>
        private void OnClose()
        {
            this.DetachFromParent();
        }

        /// <summary>
        /// Commit all changes
        /// </summary>
        /// <param name="state"></param>
        public void Commit(InstrumentationState state)
        {
            // Commit all child properties
            foreach (IInstrumentationProperty instrumentationProperty in this.GetProperties<IInstrumentationProperty>())
            {
                instrumentationProperty.Commit(state);
            }
            
            state.FeatureBitMask |= FeatureInfo.FeatureBit;
        }
    }
}