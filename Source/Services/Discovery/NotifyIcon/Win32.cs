using System;
using System.Runtime.InteropServices;

namespace Studio
{
    public static class Win32
    {
        /// <summary>
        /// Restore the target window
        /// </summary>
        public static readonly int CmdShowRestoreId = 9;
        
        /// <summary>
        /// SetForegroundWindow import
        /// </summary>
        [DllImport("User32.dll")]
        public static extern bool SetForegroundWindow(IntPtr hWnd);
        
        /// <summary>
        /// ShowWindow import
        /// </summary>
        [DllImport("User32.dll")]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
        
        /// <summary>
        /// IsIconic import
        /// </summary>
        [DllImport("User32.dll")]
        public static extern bool IsIconic(IntPtr hWnd);
    }
}