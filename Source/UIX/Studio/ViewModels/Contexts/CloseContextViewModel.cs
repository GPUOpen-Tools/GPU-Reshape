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

using System.Collections.Generic;
using ReactiveUI;
using Studio.ViewModels.Traits;

namespace Studio.ViewModels.Contexts
{
    public class CloseContextViewModel : ReactiveObject, IContextViewModel
    {
        /// <summary>
        /// Install a context against a target view model
        /// </summary>
        /// <param name="itemViewModels">destination items list</param>
        /// <param name="targetViewModel">the target to install for</param>
        public void Install(IList<IContextMenuItemViewModel> itemViewModels, object targetViewModel)
        {
            if (targetViewModel is not IClosableObject closable)
            {
                return;
            }
            
            itemViewModels.Add(new ContextMenuItemViewModel()
            {
                Header = "Close",
                Command = ReactiveCommand.Create(() => OnInvoked(closable))
            });
        }

        /// <summary>
        /// Command implementation
        /// </summary>
        private void OnInvoked(IClosableObject closable)
        {
            closable.CloseCommand?.Execute(null);
        }
    }
}