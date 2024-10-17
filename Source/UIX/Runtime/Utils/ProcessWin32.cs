using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Studio.Utils
{
    public static class ProcessWin32
    {
        /// <summary>
        /// Native PBI block
        /// </summary>
        public struct PROCESS_BASIC_INFORMATION
        {
            public int ExitStatus;
            public int Pad;
            public IntPtr PebBaseAddress;
            public IntPtr AffinityMask;
            public long BasePriority;
            public IntPtr UniqueProcessId;
            public IntPtr InheritedFromUniqueProcessId;
        }
        
        /// <summary>
        /// Import
        /// </summary>
        [DllImport("ntdll.dll", SetLastError = true)]
        public static extern int NtQueryInformationProcess(IntPtr handle, int processInformationClass, ref PROCESS_BASIC_INFORMATION processInformation, int processInformationLength, out int returnLength);

        /// <summary>
        /// Helper, gets the process info from an id 
        /// </summary>
        /// <returns>null if failed</returns>
        public static Process? GetProcess(int id)
        {
            try
            {
                return Process.GetProcessById(id);
            }
            catch (ArgumentException)
            {
                return null;
            }
        }
        
        /// <summary>
        /// Get the parent process from an id
        /// </summary>
        /// <returns>null if failed</returns>
        public static Process? GetParentProcess(int id)
        {
            PROCESS_BASIC_INFORMATION pbi = new();

            // Try to open process
            Process? process = GetProcess(id);
            if (process == null)
            {
                return null;
            }

            // Try to get PBI
            try
            {
                if (NtQueryInformationProcess(process.Handle, 0, ref pbi, Marshal.SizeOf(pbi), out int length) != 0)
                {
                    return null;
                }
            }
            catch (InvalidOperationException)
            {
                return null;
            }

            // Get from parent handle
            return GetProcess(pbi.InheritedFromUniqueProcessId.ToInt32());
        }
    }
}