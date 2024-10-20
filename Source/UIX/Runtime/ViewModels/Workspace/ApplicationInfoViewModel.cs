﻿// 
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
using ReactiveUI;

namespace Studio.ViewModels.Workspace
{
    public class ApplicationInfoViewModel : ReactiveObject
    {
        /// <summary>
        /// Process id of the application
        /// </summary>
        public ulong Pid
        {
            get => _pid;
            set => this.RaiseAndSetIfChanged(ref _pid, value);
        }
        
        /// <summary>
        /// Process id of the application
        /// </summary>
        public ulong DeviceUid
        {
            get => _deviceUid;
            set => this.RaiseAndSetIfChanged(ref _deviceUid, value);
        }
        
        /// <summary>
        /// Process id of the application
        /// </summary>
        public ulong DeviceObjects
        {
            get => _deviceObjects;
            set => this.RaiseAndSetIfChanged(ref _deviceObjects, value);
        }

        /// <summary>
        /// Underlying API of the application
        /// </summary>
        public string API
        {
            get => _api;
            set => this.RaiseAndSetIfChanged(ref _api, value);
        }

        /// <summary>
        /// Decorative name of the application
        /// </summary>
        public string Name
        {
            get => _name;
            set => this.RaiseAndSetIfChanged(ref _name, value);
        }

        /// <summary>
        /// Process name of the application
        /// </summary>
        public string Process
        {
            get => _process;
            set => this.RaiseAndSetIfChanged(ref _process, value);
        }

        /// <summary>
        /// Host resolve GUID of the application
        /// </summary>
        public Guid Guid
        {
            get => _guid;
            set => this.RaiseAndSetIfChanged(ref _guid, value);
        }

        /// <summary>
        /// The decorated presentation mode
        /// </summary>
        public ApplicationDecorationMode DecorationMode
        {
            get => _decorationMode; 
            set => this.RaiseAndSetIfChanged(ref _decorationMode, value);
        }

        /// <summary>
        /// Decorated name of the info
        /// </summary>
        public string DecoratedName
        {
            get
            {
                switch (_decorationMode)
                {
                    case ApplicationDecorationMode.Default:
                        return $"{Process} - {DecoratedDeviceName}";
                    case ApplicationDecorationMode.ProcessOnly:
                        return Process;
                    case ApplicationDecorationMode.DeviceOnly:
                        return DecoratedDeviceName;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }
        }

        /// <summary>
        /// Decorated device name of the info
        /// </summary>
        public string DecoratedDeviceName
        {
            get
            {
                if (Name.Length > 0)
                {
                    return $"{Name} {API}";
                }

                return API;
            }
        }
        
        /// <summary>
        /// Internal PID
        /// </summary>
        private ulong _pid;
        
        /// <summary>
        /// Internal UID
        /// </summary>
        private ulong _deviceUid;
        
        /// <summary>
        /// Internal objects
        /// </summary>
        private ulong _deviceObjects;
        
        /// <summary>
        /// Internal API
        /// </summary>
        private string _api = string.Empty;
        
        /// <summary>
        /// Internal Name
        /// </summary>
        private string _name = string.Empty;
        
        /// <summary>
        /// Internal Process
        /// </summary>
        private string _process = string.Empty;
        
        /// <summary>
        /// Internal decoration mode
        /// </summary>
        private ApplicationDecorationMode _decorationMode = ApplicationDecorationMode.Default;

        /// <summary>
        /// Internal GUID
        /// </summary>
        private Guid _guid;
    }
}