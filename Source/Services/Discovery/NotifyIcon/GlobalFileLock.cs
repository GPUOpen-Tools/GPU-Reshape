// 
// The MIT License (MIT)
// 
// Copyright (c) 2023 Miguel Petersen
// Copyright (c) 2023 Advanced Micro Devices, Inc
// Copyright (c) 2023 Avalanche Studios Group
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
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Principal;
using System.Threading;

namespace Studio
{
    class GlobalFileLock : IDisposable
    {
        /// <summary>
        /// Attempt to acquire the mutex
        /// </summary>
        /// <param name="domain">application domain</param>
        /// <param name="app">application name</param>
        /// <returns></returns>
        public bool Acquire(string domain = "GPUReshape", string app = "NotifyIcon")
        {
            // Unique path
            var path = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),  domain, app);
            
            // Try to create
            Directory.CreateDirectory(path);
            
            // Try to lock file
            try
            {
                _stream = File.Open(Path.Combine(path, ".lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
                _stream.Lock(0, 0);
                return true;
            }
            catch
            {
                return false;
            }
        }

        /// <summary>
        /// Invoked on disposes
        /// </summary>
        public void Dispose()
        {
            _stream?.Dispose();
        }

        /// <summary>
        /// Internal stream
        /// </summary>
        private FileStream? _stream;
    }
}