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

using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Workspace;
using System.Reactive.Linq;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.Views.Tools
{
    public partial class WorkspaceView : UserControl
    {
        public WorkspaceView()
        {
            InitializeComponent();
            
            this.WhenAnyValue(x => x.DataContext)
                .WhereNotNull()
                .Subscribe(x =>
                {
                    // Bind signals
                    WorkspaceTree.Events().DoubleTapped
                        .Select(_ => WorkspaceTree.SelectedItem as WorkspaceTreeItemViewModel)
                        .WhereNotNull()
                        .Subscribe(x => x.OpenDocument.Execute(null));
                });
        }

        private void OnItemSelected(object? sender, SelectionChangedEventArgs e)
        {
            // Validate sender
            if (e.AddedItems.Count == 0 || e.AddedItems[0] is not { } _object)
                return;
            
            // Get service
            var service = App.Locator.GetService<IWorkspaceService>();
            if (service == null)
                return;

            // Workspace?
            if (_object is WorkspaceTreeItemViewModel { OwningContext: IWorkspaceViewModel workspaceViewModel })
            {
                service.SelectedWorkspace = workspaceViewModel;
                service.SelectedProperty = workspaceViewModel.PropertyCollection;
            }

            // Property?
            if (_object is WorkspaceTreeItemViewModel { OwningContext: IPropertyViewModel propertyViewModel })
            {
                service.SelectedProperty = propertyViewModel;
            }
        }
        
        private WorkspaceViewModel? VM => DataContext as WorkspaceViewModel;
    }
}