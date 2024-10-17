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

using System.Collections.Generic;
using System.Linq;
using ReactiveUI;
using Studio.Extensions;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Contexts
{
    public class CloseContextViewModel : ReactiveObject, IContextViewModel
    {
        /// <summary>
        /// Install a context against a target view model
        /// </summary>
        /// <param name="itemViewModels">destination items list</param>
        /// <param name="targetViewModels">the targets to install for</param>
        public void Install(IList<IContextMenuItemViewModel> itemViewModels, object[] targetViewModels)
        {
            if (targetViewModels.PromoteOrNull<IClosableObject>() is not {} closable)
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
        private void OnInvoked(IClosableObject[] closables)
        {
            HashSet<IClosableObject> set = closables.ToHashSet();

            // Closable objects need a bit of care, basically, we need to make sure
            // that we're not closing anything twice in any particular chain, which
            // may happen if a parent is closed with a child.
            foreach (IClosableObject closable in closables)
            {
                // If this is not a hierarchical object, just close it immediately
                if (closable is not IPropertyViewModel property)
                {
                    closable.CloseCommand?.Execute(null);
                    continue;
                }

                // Does this object have a to be closed parent?
                bool hasParent = false;

                // Traverse up the tree to see if there's a shared parent which is to be closed
                foreach (IPropertyViewModel parent in property.GetParents())
                {
                    if (parent is IClosableObject closableParent && set.Contains(closableParent))
                    {
                        hasParent = true;
                        break;
                    }
                }

                // If not this is isolated, so just close it
                if (!hasParent)
                {
                    closable.CloseCommand?.Execute(null);
                }
            }
        }
    }
}