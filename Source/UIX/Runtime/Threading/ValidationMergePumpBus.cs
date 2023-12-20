// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc.,
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
using Studio.ViewModels.Workspace.Objects;

namespace Runtime.Threading
{
    public class ValidationMergePumpBus
    {
        /// <summary>
        /// Increment a validation object
        /// </summary>
        /// <param name="validationObject">the object</param>
        /// <param name="increment">the count to increment by</param>
        public static void Increment(ValidationObject validationObject, uint increment)
        {
            _bus.Add(() => { validationObject.IncrementCountNoRaise(increment); }, validationObject);
        }

        /// <summary>
        /// Merge the collection
        /// </summary>
        /// <param name="objects"></param>
        private static void CollectionMerge(IEnumerable<ValidationObject> objects)
        {
            foreach (ValidationObject validationObject in objects)
            {
                validationObject.NotifyCountChanged();
            }
        }

        /// <summary>
        /// Underlying bus
        /// </summary>
        private static CollectionMergePumpBus<ValidationObject> _bus = new()
        {
            CollectionMerge = CollectionMerge
        };
    }
}