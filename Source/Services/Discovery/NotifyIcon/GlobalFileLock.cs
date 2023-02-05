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