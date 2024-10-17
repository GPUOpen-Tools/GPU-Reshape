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
using System.Linq;
using Avalonia;
using Avalonia.Media;
using Dock.Model.ReactiveUI.Controls;
using DynamicData;
using ReactiveUI;
using Runtime.ViewModels.Tools;
using Studio.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Tools
{
    public class PropertyViewModel : ToolViewModel
    {
        /// <summary>
        /// Tooling icon
        /// </summary>
        public override StreamGeometry? Icon => ResourceLocator.GetIcon("ToolProperty");
        
        /// <summary>
        /// All configurations
        /// </summary>
        public IEnumerable<IPropertyViewModel>? SelectedPropertyConfigurations
        {
            get => _selectedPropertyConfigurations;
            set => this.RaiseAndSetIfChanged(ref _selectedPropertyConfigurations, value);
        }

        /// <summary>
        /// Current property description
        /// </summary>
        public string CurrentDescription
        {
            get => _currentDescription;
            set => this.RaiseAndSetIfChanged(ref _currentDescription, value);
        }

        public PropertyViewModel()
        {
            // Bind to selected property
            ServiceRegistry.Get<IWorkspaceService>()?
                .WhenAnyValue(x => x.SelectedProperty)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Filter by configurations
                    SelectedPropertyConfigurations = x.Properties.Items.Where(p => p.Visibility == PropertyVisibility.Configuration);
                });
        }

        /// <summary>
        /// Internal configurations
        /// </summary>
        private IEnumerable<IPropertyViewModel>? _selectedPropertyConfigurations;

        /// <summary>
        /// Internal description
        /// </summary>
        private string _currentDescription = string.Empty;
    }
}
