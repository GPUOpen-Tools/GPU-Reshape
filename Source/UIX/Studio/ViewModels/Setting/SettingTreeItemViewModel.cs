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
using System.Collections.ObjectModel;
using System.Linq;
using DynamicData;
using DynamicData.Binding;
using ReactiveUI;
using Studio.ViewModels.Controls;

namespace Studio.ViewModels.Setting
{
    public class SettingTreeItemViewModel : ReactiveObject
    {
        /// <summary>
        /// Display header of this settings item
        /// </summary>
        public string Header
        {
            get => _header;
            set => this.RaiseAndSetIfChanged(ref _header, value);
        }

        /// <summary>
        /// Underlying setting
        /// </summary>
        public ISettingViewModel? Setting
        {
            get => _setting;
            set
            {
                this.RaiseAndSetIfChanged(ref _setting, value);
                OnSettingChanged();
            }
        }

        /// <summary>
        /// Optional dictionary for association
        /// </summary>
        public ObjectDictionary<ISettingViewModel, SettingTreeItemViewModel>? Dictionary { get; set; }

        /// <summary>
        /// Items within this settings item
        /// </summary>
        public ObservableCollection<SettingTreeItemViewModel> Items { get; } = new();

        /// <summary>
        /// Is this setting item expanded?
        /// </summary>
        public bool IsExpanded
        {
            get => _isExpanded;
            set => this.RaiseAndSetIfChanged(ref _isExpanded, value);
        }

        /// <summary>
        /// Invoked on settings change
        /// </summary>
        private void OnSettingChanged()
        {
            if (_setting == null)
            {
                return;
            }

            // Bind header
            _setting.WhenAnyValue(x => x.Header).Subscribe(x => Header = x);

            // Bind items
            _setting.Items.ToObservableChangeSet()
                .OnItemAdded(OnItemAdded)
                .OnItemRemoved(OnItemRemoved)
                .Subscribe();
        }

        /// <summary>
        /// Invoked on item additions
        /// </summary>
        /// <param name="obj"></param>
        private void OnItemAdded(ISettingViewModel obj)
        {
            if (obj.Visibility.HasFlag(SettingVisibility.Tool))
            {
                Items.Add(new SettingTreeItemViewModel()
                {
                    Dictionary = Dictionary,
                    Setting = obj
                });
                
                Dictionary?.Add(obj, Items.Last());
            }
        }

        /// <summary>
        /// Invoked on item removal
        /// </summary>
        /// <param name="obj"></param>
        private void OnItemRemoved(ISettingViewModel obj)
        {
            SettingTreeItemViewModel item = Items.First(x => x.Setting == obj);
            Dictionary?.Remove(obj, item);
            Items.Remove(item);
        }

        /// <summary>
        /// Internal header
        /// </summary>
        private string _header = string.Empty;
        
        /// <summary>
        /// Internal setting
        /// </summary>
        private ISettingViewModel? _setting;
        
        /// <summary>
        /// Internal expansion state
        /// </summary>
        private bool _isExpanded = true;
    }
}