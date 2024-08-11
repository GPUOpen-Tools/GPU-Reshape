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

using System;
using Avalonia.Threading;
using Bridge.CLR;
using Message.CLR;
using ReactiveUI;
using Studio.Models.Instrumentation;
using Studio.ViewModels.Traits;
using Studio.ViewModels.Workspace;
using Studio.ViewModels.Workspace.Properties;

namespace Runtime.ViewModels.Workspace.Properties.Bus
{
    public class InstrumentationBusVersionController : ReactiveObject, IBusVersionController, IBridgeListener
    {
        /// <summary>
        /// Null version id
        /// </summary>
        public static readonly InstrumentationVersion NoVersionID = new InstrumentationVersion()
        {
            ID = 0
        };
        
        /// <summary>
        /// Next version to be committed
        /// </summary>
        public InstrumentationVersion NextVersionID
        {
            get => _nextVersionId;
            set => this.RaiseAndSetIfChanged(ref _nextVersionId, value);
        }
        
        /// <summary>
        /// Known committed version
        /// </summary>
        public InstrumentationVersion CommittedVersionID
        {
            get => _committedVersionID;
            set => this.RaiseAndSetIfChanged(ref _committedVersionID, value);
        }
        
        /// <summary>
        /// View model associated with this service
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel
        {
            get => _connectionViewModel;
            set => this.RaiseAndSetIfChanged(ref _connectionViewModel, value);
        }
        
        /// <summary>
        /// Constructor
        /// </summary>
        public InstrumentationBusVersionController()
        {
            this.WhenAnyValue(x => x.ConnectionViewModel).Subscribe(_ => OnConnectionChanged());
        }
        
        /// <summary>
        /// Invoked when a connection has changed
        /// </summary>
        private void OnConnectionChanged()
        {
            // Register to messages
            ConnectionViewModel?.Bridge?.Register(this);
            ConnectionViewModel?.Bridge?.Register(LogMessage.ID, this);
        }

        /// <summary>
        /// Invoked on destruction
        /// </summary>
        public void Destruct()
        {
            // Remove listeners
            ConnectionViewModel?.Bridge?.Deregister(LogMessage.ID, this);
        }
        
        /// <summary>
        /// Bridge handler
        /// </summary>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            UInt64 version = 0;

            // Relevant stream?
            if (!streams.GetSchema().IsOrdered())
            {
                return;
            }

            // Find version message
            foreach (OrderedMessage message in new OrderedMessageView(streams))
            {
                switch (message.ID)
                {
                    case InstrumentationVersionMessage.ID:
                        version = message.Get<InstrumentationVersionMessage>().version;
                        break;
                }
            }

            // Commit on main thread if anything has changed
            if (version != 0)
            {
                Dispatcher.UIThread.InvokeAsync(() =>
                {
                    CommittedVersionID = new InstrumentationVersion()
                    {
                        ID = version
                    };
                });
            }
        }

        /// <summary>
        /// Commit this version controller
        /// </summary>
        public void Commit(OrderedMessageView<ReadWriteMessageStream> stream)
        {
            var message = stream.Add<InstrumentationVersionMessage>();
            message.version = NextVersionID.ID;

            NextVersionID = new InstrumentationVersion()
            {
                ID = NextVersionID.ID + 1
            };
        }

        /// <summary>
        /// Internal version id
        /// </summary>
        private InstrumentationVersion _nextVersionId = new InstrumentationVersion()
        {
            ID = 1
        };

        /// <summary>
        /// Internal connection
        /// </summary>
        private IConnectionViewModel? _connectionViewModel;

        /// <summary>
        /// Internal known committed version
        /// </summary>
        private InstrumentationVersion _committedVersionID;
    }

    public static class InstrumentationBusVersionControllerExtensions
    {
        /// <summary>
        /// Get the version controller of a property
        /// </summary>
        public static InstrumentationBusVersionController? GetInstrumentationVersionController(this IPropertyViewModel self)
        {
            return self.GetBusService()?.GetVersionController<InstrumentationBusVersionController>();
        }

        /// <summary>
        /// Get the next commit version of a property
        /// </summary>
        public static InstrumentationVersion GetNextInstrumentationVersion(this IPropertyViewModel self)
        {
            return self.GetInstrumentationVersionController()?.NextVersionID ?? InstrumentationBusVersionController.NoVersionID;
        }
    }
}
