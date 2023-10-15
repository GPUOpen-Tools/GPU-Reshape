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
using System.Diagnostics;
using System.Reactive;
using System.Reactive.Subjects;
using Avalonia;
using Avalonia.Controls.Notifications;
using Avalonia.Platform;
using Avalonia.Threading;
using Message.CLR;
using ReactiveUI;
using Runtime.ViewModels.Traits;
using Studio.Models.Workspace;

namespace Studio.ViewModels.Workspace
{
    public interface IConnectionViewModel : IDestructableObject
    {
        /// <summary>
        /// Associated application information
        /// </summary>
        public ApplicationInfo? Application { get; }
        
        /// <summary>
        /// Invoked during connection
        /// </summary>
        public ISubject<Unit> Connected { get; }
        
        /// <summary>
        /// Invoked during connection rejection
        /// </summary>
        public ISubject<Unit> Refused { get; }
        
        /// <summary>
        /// Bridge within this connection
        /// </summary>
        public Bridge.CLR.IBridge? Bridge { get; }

        /// <summary>
        /// Time on the remote endpoint
        /// </summary>
        public DateTime LocalTime { get; set; }
        
        /// <summary>
        /// Get the shared bus
        /// </summary>
        /// <returns></returns>
        public OrderedMessageView<ReadWriteMessageStream> GetSharedBus();

        /// <summary>
        /// Commit all pending messages
        /// </summary>
        public void Commit();
    }
}