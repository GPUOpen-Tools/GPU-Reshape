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

using System.Collections.ObjectModel;
using System.Windows.Input;

namespace Studio.ViewModels.Contexts
{
    public interface IContextMenuItemViewModel
    {
        /// <summary>
        /// Display header of this menu item
        /// </summary>
        public string Header { get; set; }
        
        /// <summary>
        /// Items within this menu item
        /// </summary>
        public ObservableCollection<IContextMenuItemViewModel> Items { get; }
        
        /// <summary>
        /// Target command
        /// </summary>
        public ICommand? Command { get; }

        /// <summary>
        /// Is this menu item enabled?
        /// </summary>
        public bool IsVisible { get; set; }

        /// <summary>
        /// Target view models of the context
        /// </summary>
        public object[]? TargetViewModels { get; set; }
    }

    public static class ContextMenuItemExtensions
    {
        /// <summary>
        /// Get an item from this context menu item
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetItem<T>(this IContextMenuItemViewModel self) where T : IContextMenuItemViewModel
        {
            foreach (IContextMenuItemViewModel contextMenuItemViewModel in self.Items)
            {
                if (contextMenuItemViewModel is T typed)
                {
                    return typed;
                }
            }

            return default;
        }
        
        public static IContextMenuItemViewModel GetOrAddCategory(this IContextMenuItemViewModel self, string name)
        {
            foreach (IContextMenuItemViewModel contextMenuItemViewModel in self.Items)
            {
                if (contextMenuItemViewModel.Header == name)
                {
                    return contextMenuItemViewModel;
                }
            }

            var item = new CategoryContextMenuItemViewModel()
            {
                Header = name
            };
            
            self.Items.Add(item);
            return item;
        }
    }
}