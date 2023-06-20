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

using System;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.VisualTree;
using ReactiveUI;
using Studio.ViewModels.Setting;

namespace Studio.Views.Setting
{
    public partial class PDBSettingView : UserControl, IViewFor
    {
        public object? ViewModel { get; set; }

        public PDBSettingView()
        {
            InitializeComponent();

            // Bind events
            AddButton.Events().Click.Subscribe(OnAddButton);
        }
        
        /// <summary>
        /// Invoked on add button
        /// </summary>
        /// <param name="x"></param>
        private async void OnAddButton(RoutedEventArgs x)
        {
            // Create dialog
            var dialog = new OpenFolderDialog();

            // Get requested folder
            string? result = await dialog.ShowAsync((Window)this.GetVisualRoot());
            if (result == null)
            {
                return;
            }

            // Add directory to search directories
            if (DataContext is PDBSettingViewModel vm)
            {
                vm.SearchDirectories.Add(result);
            }
        }
    }
}