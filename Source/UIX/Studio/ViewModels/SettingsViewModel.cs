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
using ReactiveUI;
using Studio.Services;
using Studio.ViewModels.Controls;
using Studio.ViewModels.Setting;

namespace Studio.ViewModels
{
    public class SettingsViewModel : ReactiveObject
    {
        /// <summary>
        /// The root settings view model
        /// </summary>
        public ISettingViewModel SettingViewModel { get; }
        
        /// <summary>
        /// The root tree-wise item view model
        /// </summary>
        public SettingTreeItemViewModel TreeItemViewModel { get; }

        /// <summary>
        /// Currently selected setting
        /// </summary>
        public ISettingViewModel? SelectedSettingViewModel
        {
            get => SelectedSettingItem == null ? null : _dictionary.Get(SelectedSettingItem);
            set => SelectedSettingItem = value != null ? _dictionary.Get(value) : null;
        }

        /// <summary>
        /// Currently selected setting
        /// </summary>
        public SettingTreeItemViewModel? SelectedSettingItem
        {
            get => _selectedSettingItem;
            set => this.RaiseAndSetIfChanged(ref _selectedSettingItem, value);
        }
        
        public SettingsViewModel()
        {
            // Get service
            var settingsService = ServiceRegistry.Get<ISettingsService>() ?? throw new InvalidOperationException();

            // Set root view model
            SettingViewModel = settingsService.ViewModel;

            // Create tree item
            TreeItemViewModel = new SettingTreeItemViewModel()
            {
                Dictionary = _dictionary,
                Setting = SettingViewModel
            };

            // Assume first item
            SelectedSettingItem = TreeItemViewModel.Items[0];
        }

        /// <summary>
        /// Internal selected setting
        /// </summary>
        private SettingTreeItemViewModel? _selectedSettingItem;

        /// <summary>
        /// Association between view models and tree items
        /// </summary>
        private ObjectDictionary<ISettingViewModel, SettingTreeItemViewModel> _dictionary = new();
    }
}
