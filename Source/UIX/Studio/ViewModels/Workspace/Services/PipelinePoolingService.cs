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
using System.Collections.Generic;
using Avalonia.Threading;
using Message.CLR;
using Studio.Models.Workspace.Objects;
using Studio.ViewModels.Workspace.Objects;

namespace Studio.ViewModels.Workspace.Services
{
    public class PipelinePoolingService : IPropertyService, IPipelinePoolingService, Bridge.CLR.IBridgeListener
    {
        /// <summary>
        /// Connection for this listener
        /// </summary>
        public IConnectionViewModel? ConnectionViewModel { get; set; }

        /// <summary>
        /// Bridge handler
        /// </summary>
        /// <param name="streams"></param>
        /// <param name="count"></param>
        public void Handle(ReadOnlyMessageStream streams, uint count)
        {
            if (!streams.GetSchema().IsOrdered())
            {
                return;
            }

            // Create view
            var view = new OrderedMessageView(streams);

            // Consume all messages
            lock (this)
            {
                foreach (OrderedMessage message in view)
                {
                    switch (message.ID)
                    {
                        case PipelineNameMessage.ID:
                        {
                            var typed = message.Get<PipelineNameMessage>();

                            // Try to get the view model
                            if (!_pendingPipelineViewModels.TryGetValue(typed.pipelineUID, out PendingEntry? entry) || entry.PipelineViewModel == null)
                            {
                                Studio.Logging.Error($"Failed to get pipeline view model for {typed.pipelineUID}");
                                continue;
                            }

                            // Flatten name
                            string name = typed.name.String;
                            
                            // Update status
                            Dispatcher.UIThread.InvokeAsync(() =>
                            {
                                entry.PipelineViewModel.AsyncStatus = AsyncObjectStatus.DebugSymbols;
                                entry.PipelineViewModel.Name = name;
                            });
                            break;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Enqueue a request for a pipeline
        /// </summary>
        /// <param name="pipelineViewModel"></param>
        public void EnqueuePipeline(PipelineViewModel pipelineViewModel)
        {
            lock (this)
            {
                // Get entry
                PendingEntry entry = GetEntry(pipelineViewModel.GUID);

                // Valid or already pooled?
                if (ConnectionViewModel == null || entry.State.HasFlag(PipelinePoolingState.Name))
                {
                    return;
                }

                // Update entry
                entry.State |= PipelinePoolingState.Name;
                entry.PipelineViewModel = pipelineViewModel;

                // Add request
                var request = ConnectionViewModel.GetSharedBus().Add<GetPipelineNameMessage>();
                request.pipelineUID = pipelineViewModel.GUID;
            }
        }

        /// <summary>
        /// Get an existing entry
        /// </summary>
        /// <param name="guid"></param>
        /// <returns></returns>
        private PendingEntry GetEntry(UInt64 guid)
        {
            if (!_pendingPipelineViewModels.TryGetValue(guid, out PendingEntry? entry))
            {
                entry = new PendingEntry()
                {
                    State = PipelinePoolingState.None
                };

                // Populate missing entry
                _pendingPipelineViewModels.Add(guid, entry);
            }

            return entry;
        }

        /// <summary>
        /// Internal pending entry
        /// </summary>
        private class PendingEntry
        {
            /// <summary>
            /// Current pooling state
            /// </summary>
            public PipelinePoolingState State;

            /// <summary>
            /// Assume there's always an external reference, up to the caller if this is valid
            /// </summary>
            public bool HasExternalReference = true;

            /// <summary>
            /// Parent view model
            /// </summary>
            public PipelineViewModel? PipelineViewModel;
        }

        /// <summary>
        /// All pending view models, i.e. in-flight
        /// </summary>
        private Dictionary<UInt64, PendingEntry> _pendingPipelineViewModels = new();
    }
}