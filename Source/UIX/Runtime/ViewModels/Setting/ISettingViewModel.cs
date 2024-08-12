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
using System.Collections.ObjectModel;

namespace Studio.ViewModels.Setting
{
    public interface ISettingViewModel
    {
        /// <summary>
        /// Display header of this settings item
        /// </summary>
        public string Header { get; set; }
        
        /// <summary>
        /// Items within this settings item
        /// </summary>
        public ObservableCollection<ISettingViewModel> Items { get; }

        /// <summary>
        /// Given visibility of this view model
        /// </summary>
        public SettingVisibility Visibility { get; }
        
    }

    public static class SettingItemExtensions
    {
        /// <summary>
        /// Get an item from this context settings item
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetItem<T>(this ISettingViewModel self) where T : ISettingViewModel
        {
            foreach (ISettingViewModel settingItemViewModel in self.Items)
            {
                if (settingItemViewModel is T typed)
                {
                    return typed;
                }
            }

            return default;
        }

        /// <summary>
        /// Get an item from this context settings item which matches a predicate
        /// </summary>
        /// <returns>null if not found</returns>
        public static T? GetFirstItem<T>(this ISettingViewModel self, Func<T, bool> predicate) where T : ISettingViewModel
        {
            foreach (ISettingViewModel settingItemViewModel in self.Items)
            {
                if (settingItemViewModel is T typed && predicate(typed))
                {
                    return typed;
                }
            }

            return default;
        }

        /// <summary>
        /// Get an enumerable on settings of type
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        public static IEnumerable<T> Where<T>(this ISettingViewModel self) where T : class
        {
            foreach (ISettingViewModel settingViewModel in self.Items)
            {
                if (settingViewModel is T typed)
                {
                    yield return typed;
                }
            }
        }
    }
}