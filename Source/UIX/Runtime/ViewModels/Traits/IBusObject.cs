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
using Avalonia;
using Message.CLR;
using Runtime.ViewModels.Traits;
using Runtime.ViewModels.Workspace.Properties;
using Studio.Services;
using Studio.ViewModels.Workspace.Properties;

namespace Studio.ViewModels.Traits
{
    public interface IBusObject
    {
        /// <summary>
        /// Commit all state to a given stream
        /// </summary>
        /// <param name="stream"></param>
        public void Commit(OrderedMessageView<ReadWriteMessageStream> stream);
    }

    public static class BusObjectExtensions
    {
        /// <summary>
        /// Get the busd service of a property
        /// </summary>
        public static IBusPropertyService? GetBusService(this IPropertyViewModel self)
        {
            return self.GetWorkspaceCollection()?.GetService<IBusPropertyService>();
        }
        
        /// <summary>
        /// Enqueue a bus object from a given property view model
        /// </summary>
        /// <param name="self"></param>
        /// <param name="propertyViewModel"></param>
        public static void EnqueueBus(this IBusObject self, IPropertyViewModel propertyViewModel)
        {
            propertyViewModel.GetWorkspaceCollection()?.GetService<IBusPropertyService>()?.Enqueue(self);
        }
        
        /// <summary>
        /// Enqueue a bus object from a property view model
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        public static void EnqueueBus<T>(this T self) where T : IBusObject, IPropertyViewModel
        {
            self.EnqueueBus(self);
        }
        
        /// <summary>
        /// Enqueue a bus object from the first appropriate property view model
        /// </summary>
        /// <param name="self"></param>
        /// <typeparam name="T"></typeparam>
        public static bool EnqueueFirstParentBus<T>(this T self) where T : IPropertyViewModel
        {
            IPropertyViewModel top = self;

            // Walk up until we reach the first bus object
            while (top != null)
            {
                if (top is IBusObject busObject)
                {
                    busObject.EnqueueBus(top);
                    return true;
                }
                
                top = top.Parent;
            }
            
            // None found
            return false;
        }
    }
}
