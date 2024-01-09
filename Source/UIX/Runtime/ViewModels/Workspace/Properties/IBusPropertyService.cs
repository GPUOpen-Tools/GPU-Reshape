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

using DynamicData;
using Message.CLR;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;

namespace Runtime.ViewModels.Workspace.Properties
{
    public interface IBusPropertyService : IPropertyService
    {
        /// <summary>
        /// Current bus mode
        /// </summary>
        public BusMode Mode { get; set; }
        
        /// <summary>
        /// All outstanding requests
        /// </summary>
        public ISourceList<IBusObject> Objects { get; }

        /// <summary>
        /// Enqueue a bus object, enqueuing guaranteed to be unique
        /// </summary>
        /// <param name="busObject"></param>
        public void Enqueue(IBusObject busObject);

        /// <summary>
        /// Commit all enqueued bus objects
        /// </summary>
        public void Commit();

        /// <summary>
        /// Commit all enqueued bus objects
        /// </summary>
        public void CommitRedirect(OrderedMessageView<ReadWriteMessageStream> stream, bool flush);
    }
}