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

using System;

namespace Runtime.ViewModels.Workspace.Properties
{
    public class BusScope : IDisposable
    {
        /// <summary>
        /// Create a bus scope
        /// </summary>
        /// <param name="service">target service</param>
        /// <param name="mode">temporary scope</param>
        public BusScope(IBusPropertyService? service, BusMode mode)
        {
            _service = service;

            // Null allowed
            if (service == null)
            {
                return;
            }
            
            // Cache and set scope
            _reconstruct = service.Mode;
            service.Mode = mode;
        }
        
        /// <summary>
        /// Dispose the scope
        /// </summary>
        public void Dispose()
        {
            if (_service == null)
            {
                return;
            }
            
            // Commit pending if requested
            if (_service.Mode == BusMode.RecordAndCommit && _reconstruct != BusMode.RecordAndCommit)
            {
                _service.Commit();
            }

            // Reconstruct scope
            _service.Mode = _reconstruct;
        }

        /// <summary>
        /// Internal service
        /// </summary>
        private IBusPropertyService? _service;
        
        /// <summary>
        /// Internal reconstruction state
        /// </summary>
        private BusMode _reconstruct;
    }
}