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

using System;
using Message.CLR;

namespace Runtime.Models.Objects
{
    public class InstrumentationState
    {
        /// <summary>
        /// Instrumentation bit mask, see feature infos
        /// </summary>
        public UInt64 FeatureBitMask { get; set; }
        
        /// <summary>
        /// Instrumentation specialization stream
        /// </summary>
        public OrderedMessageView<ReadWriteMessageStream> SpecializationStream { get; set; }
        
        /// <summary>
        /// Get a specialization state or create a default value
        /// </summary>
        public T GetOrDefault<T>() where T : struct, IMessage
        {
            // Create default request
            IMessageAllocationRequest request = new T().DefaultRequest();
            
            // Already exists?
            foreach (OrderedMessage existing in SpecializationStream)
            {
                if (existing.ID == request.ID)
                {
                    return existing.Get<T>();
                }
            }

            // Allocate new message
            T message = SpecializationStream.Add<T>(request);
            request.Default(message);
            return message;
        }
    }
}
