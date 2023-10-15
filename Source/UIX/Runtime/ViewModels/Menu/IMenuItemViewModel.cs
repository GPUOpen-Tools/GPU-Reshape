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

using System.Collections.ObjectModel;
using System.Windows.Input;
using Avalonia.Media;

namespace Studio.ViewModels.Menu
{
    public interface IMenuItemViewModel
    {
        /// <summary>
        /// Display header of this menu item
        /// </summary>
        public string Header { get; set; }
        
        /// <summary>
        /// Items within this menu item
        /// </summary>
        public ObservableCollection<IMenuItemViewModel> Items { get; }
        
        /// <summary>
        /// Target command
        /// </summary>
        public ICommand? Command { get; }
        
        /// <summary>
        /// Is this menu item enabled?
        /// </summary>
        public bool IsEnabled { get; set; }

        /// <summary>
        /// Icon for this item
        /// </summary>
        public StreamGeometry? Icon { get; }
    }

    public static class MenuItemExtensions
    {
        /// <summary>
        /// Get an item from this context menu item
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns>null if not found</returns>
        public static T? GetItem<T>(this IMenuItemViewModel self) where T : IMenuItemViewModel
        {
            foreach (IMenuItemViewModel menuItemViewModel in self.Items)
            {
                if (menuItemViewModel is T typed)
                {
                    return typed;
                }
            }

            return default;
        }
    }
}