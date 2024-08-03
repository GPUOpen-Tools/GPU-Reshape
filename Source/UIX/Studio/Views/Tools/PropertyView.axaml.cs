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
using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Tools;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.Views.Tools
{
    public partial class PropertyView : UserControl
    {
        public PropertyView()
        {
            InitializeComponent();

            // Bind context
            this.WhenAnyValue(x => x.DataContext)
                .CastNullable<PropertyViewModel>()
                .Subscribe(x =>
                {
                    // Bind configurations
                    x.WhenAnyValue(y => y.SelectedPropertyConfigurations)
                        .Subscribe(y =>
                        {
#if false
                            // Create new descriptor
                            PropertyGrid.SelectedObject = new Property.PropertyCollectionTypeDescriptor(y ?? Array.Empty<IPropertyViewModel>());
                            
                            // Clear previous metadata, internally a type based cache is used, this is not appropriate
                            MetadataRepository.Clear();
                            
                            // Finally, recreate the property setup with fresh metadata
                            PropertyGrid.ReloadCommand.Execute(null);
                            
                            // Expand all categories
                            PropertyGrid.Categories.ForEach(c => c.IsExpanded = true);
#endif
                        });
                });
        }
    }
}